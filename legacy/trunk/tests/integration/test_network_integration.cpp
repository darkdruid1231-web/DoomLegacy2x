//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by Doom Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Integration tests for network connectivity and packet exchange.

#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <cstdint>
#include <queue>

using namespace std;

static int tests_run = 0;
static int tests_passed = 0;
static string last_failure;

#define TEST(name) do { \
    tests_run++; \
    last_failure = ""; \
    cout << "  " << name << " ... "; \
} while(0)

#define PASS() do { \
    tests_passed++; \
    cout << "PASS" << endl; \
} while(0)

#define FAIL(msg) do { \
    last_failure = msg; \
    cout << "FAIL: " << msg << endl; \
    return; \
} while(0)

#define CHECK(cond, msg) do { \
    if (!(cond)) FAIL(msg); \
} while(0)

#define CHECK_EQ(actual, expected, msg) do { \
    if ((actual) != (expected)) { \
        FAIL(string(msg) + " (expected " + to_string(expected) + ", got " + to_string(actual) + ")"); \
    } \
} while(0)

//============================================================================
// Mock Network Types
//============================================================================

// Mock address type for network testing
struct NetworkAddress {
    uint32_t ip;
    uint16_t port;
    
    NetworkAddress(uint32_t i = 0, uint16_t p = 0) : ip(i), port(p) {}
    
    bool operator==(const NetworkAddress& other) const {
        return ip == other.ip && port == other.port;
    }
    
    bool operator<(const NetworkAddress& other) const {
        if (ip != other.ip) return ip < other.ip;
        return port < other.port;
    }
};

// Mock packet type for network testing
class NetworkPacket {
public:
    NetworkAddress source;
    NetworkAddress destination;
    std::vector<uint8_t> data;
    uint32_t sequence_num;
    
    NetworkPacket() : sequence_num(0) {}
    
    NetworkPacket(const NetworkAddress& src, const NetworkAddress& dst, const uint8_t* buf, size_t len)
        : source(src), destination(dst), sequence_num(0) {
        data.assign(buf, buf + len);
    }
    
    size_t getSize() const { return data.size(); }
    
    uint8_t getByte(size_t offset) const {
        if (offset < data.size()) return data[offset];
        return 0;
    }
};

// Mock connection state
enum ConnectionState {
    CON_DISCONNECTED = 0,
    CON_CONNECTING = 1,
    CON_CONNECTED = 2,
    CON_DISCONNECTING = 3
};

//============================================================================
// Mock Network Interface
//============================================================================

class MockNetworkInterface {
private:
    std::queue<NetworkPacket> incoming_packets;
    std::queue<NetworkPacket> outgoing_packets;
    ConnectionState state;
    NetworkAddress local_addr;
    NetworkAddress remote_addr;
    uint32_t next_sequence;
    bool packet_loss_enabled;
    double packet_loss_rate;

public:
    MockNetworkInterface(uint32_t ip = 0x7F000001, uint16_t port = 10666) 
        : state(CON_DISCONNECTED), local_addr(ip, port), next_sequence(0),
          packet_loss_enabled(false), packet_loss_rate(0.0) {}
    
    void reset() {
        while (!incoming_packets.empty()) incoming_packets.pop();
        while (!outgoing_packets.empty()) outgoing_packets.pop();
        state = CON_DISCONNECTED;
        next_sequence = 0;
    }
    
    void setConnectionState(ConnectionState new_state) {
        state = new_state;
    }
    
    ConnectionState getConnectionState() const {
        return state;
    }
    
    void setRemoteAddress(const NetworkAddress& addr) {
        remote_addr = addr;
    }
    
    NetworkAddress getLocalAddress() const {
        return local_addr;
    }
    
    NetworkAddress getRemoteAddress() const {
        return remote_addr;
    }
    
    // Simulate sending a packet
    bool sendPacket(const NetworkAddress& dest, const uint8_t* data, size_t len) {
        if (packet_loss_enabled && (rand() % 100) < (packet_loss_rate * 100)) {
            return false;  // Simulate packet loss
        }
        
        NetworkPacket pkt(local_addr, dest, data, len);
        pkt.sequence_num = next_sequence++;
        outgoing_packets.push(pkt);
        return true;
    }
    
    // Simulate receiving a packet
    bool receivePacket(NetworkPacket& pkt) {
        if (incoming_packets.empty()) {
            return false;
        }
        pkt = incoming_packets.front();
        incoming_packets.pop();
        return true;
    }
    
    // Queue a packet for reception (simulating network)
    void queueIncomingPacket(const NetworkPacket& pkt) {
        incoming_packets.push(pkt);
    }
    
    size_t getOutgoingQueueSize() const {
        return outgoing_packets.size();
    }
    
    size_t getIncomingQueueSize() const {
        return incoming_packets.size();
    }
    
    void enablePacketLoss(double rate) {
        packet_loss_enabled = true;
        packet_loss_rate = rate;
    }
    
    void disablePacketLoss() {
        packet_loss_enabled = false;
    }
    
    void processOutgoing() {
        // Simulate processing - move outgoing to "network"
        while (!outgoing_packets.empty()) {
            outgoing_packets.pop();
        }
    }
};

//============================================================================
// Mock Connection (simplified LConnection)
//============================================================================

class MockConnection {
public:
    ConnectionState state;
    NetworkAddress server_addr;
    NetworkAddress client_addr;
    uint32_t connection_id;
    uint32_t ping;
    std::vector<uint8_t> receive_buffer;
    std::vector<uint8_t> send_buffer;
    bool is_server;
    
    MockConnection() : state(CON_DISCONNECTED), connection_id(0), ping(0), is_server(false) {}
    
    void reset() {
        state = CON_DISCONNECTED;
        connection_id = 0;
        ping = 0;
        receive_buffer.clear();
        send_buffer.clear();
    }
    
    bool connect(const NetworkAddress& addr) {
        if (state != CON_DISCONNECTED) {
            return false;
        }
        server_addr = addr;
        state = CON_CONNECTING;
        return true;
    }
    
    bool accept(const NetworkAddress& addr) {
        if (state != CON_DISCONNECTED) {
            return false;
        }
        client_addr = addr;
        state = CON_CONNECTED;
        connection_id = 0x12345678;
        return true;
    }
    
    void disconnect() {
        state = CON_DISCONNECTING;
        // Simulate cleanup delay
        state = CON_DISCONNECTED;
    }
    
    bool isConnected() const {
        return state == CON_CONNECTED;
    }
    
    uint32_t getPing() const {
        return ping;
    }
    
    void setPing(uint32_t p) {
        ping = p;
    }
};

//============================================================================
// Connection Establishment Tests
//============================================================================

void test_connection_disconnected_initial_state() {
    TEST("connection_disconnected_initial_state");
    MockConnection conn;
    
    CHECK(conn.state == CON_DISCONNECTED, "Initial state should be disconnected");
    CHECK(!conn.isConnected(), "Should not be connected initially");
    CHECK(conn.getPing() == 0, "Initial ping should be 0");
    PASS();
}

void test_connection_initiate() {
    TEST("connection_initiate");
    MockConnection conn;
    NetworkAddress server(0x7F000001, 10666);  // localhost
    
    bool result = conn.connect(server);
    
    CHECK(result == true, "Connect should succeed");
    CHECK(conn.state == CON_CONNECTING, "State should be connecting");
    CHECK(conn.server_addr.ip == server.ip, "Server IP should be set");
    CHECK(conn.server_addr.port == server.port, "Server port should be set");
    PASS();
}

void test_connection_accept() {
    TEST("connection_accept");
    MockConnection server_conn;
    NetworkAddress client(0x7F000002, 10667);  // different client
    
    bool result = server_conn.accept(client);
    
    CHECK(result == true, "Accept should succeed");
    CHECK(server_conn.state == CON_CONNECTED, "State should be connected");
    CHECK(server_conn.client_addr.ip == client.ip, "Client IP should be set");
    CHECK(server_conn.connection_id != 0, "Connection ID should be assigned");
    PASS();
}

void test_connection_disconnect() {
    TEST("connection_disconnect");
    MockConnection conn;
    NetworkAddress server(0x7F000001, 10666);
    
    // Connect first
    conn.connect(server);
    CHECK(conn.state == CON_CONNECTING, "Should be connecting");
    
    // Now disconnect
    conn.disconnect();
    CHECK(conn.state == CON_DISCONNECTED, "Should be disconnected after disconnect");
    CHECK(!conn.isConnected(), "Should not be connected");
    PASS();
}

void test_connection_double_connect_fails() {
    TEST("connection_double_connect_fails");
    MockConnection conn;
    NetworkAddress server1(0x7F000001, 10666);
    NetworkAddress server2(0x7F000001, 10667);
    
    bool first = conn.connect(server1);
    CHECK(first == true, "First connect should succeed");
    
    bool second = conn.connect(server2);
    CHECK(second == false, "Second connect should fail");
    CHECK(conn.state == CON_CONNECTING, "State should still be connecting");
    PASS();
}

void test_connection_from_connected_fails() {
    TEST("connection_from_connected_fails");
    MockConnection conn;
    NetworkAddress server(0x7F000001, 10666);
    
    conn.connect(server);
    conn.state = CON_CONNECTED;  // Simulate established
    
    NetworkAddress other(0x7F000001, 10667);
    bool result = conn.connect(other);
    
    CHECK(result == false, "Connect from connected state should fail");
    PASS();
}

void test_connection_accept_from_disconnected() {
    TEST("connection_accept_from_disconnected");
    MockConnection conn;
    NetworkAddress client(0x7F000002, 10667);
    
    bool result = conn.accept(client);
    
    CHECK(result == true, "Accept should succeed");
    CHECK(conn.state == CON_CONNECTED, "Should be connected");
    PASS();
}

void test_connection_multiple_clients() {
    TEST("connection_multiple_clients");
    MockConnection server1, server2;
    NetworkAddress client1(0x7F000001, 10666);
    NetworkAddress client2(0x7F000001, 10667);
    
    bool accept1 = server1.accept(client1);
    bool accept2 = server2.accept(client2);
    
    CHECK(accept1 == true, "First accept should succeed");
    CHECK(accept2 == true, "Second accept should succeed");
    CHECK(server1.isConnected(), "First server should be connected");
    CHECK(server2.isConnected(), "Second server should be connected");
    CHECK(server1.connection_id != server2.connection_id, "Connection IDs should be unique");
    PASS();
}

void test_connection_ping_measurement() {
    TEST("connection_ping_measurement");
    MockConnection conn;
    
    conn.setPing(50);
    CHECK(conn.getPing() == 50, "Ping should be 50");
    
    conn.setPing(100);
    CHECK(conn.getPing() == 100, "Ping should update to 100");
    
    conn.setPing(0);
    CHECK(conn.getPing() == 0, "Ping should be 0");
    PASS();
}

//============================================================================
// Network Interface Tests
//============================================================================

void test_network_interface_initial_state() {
    TEST("network_interface_initial_state");
    MockNetworkInterface net;
    
    CHECK(net.getConnectionState() == CON_DISCONNECTED, "Initial state should be disconnected");
    CHECK(net.getOutgoingQueueSize() == 0, "Outgoing queue should be empty");
    CHECK(net.getIncomingQueueSize() == 0, "Incoming queue should be empty");
    PASS();
}

void test_network_interface_send_packet() {
    TEST("network_interface_send_packet");
    MockNetworkInterface net;
    NetworkAddress dest(0x7F000001, 10667);
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    
    bool result = net.sendPacket(dest, data, sizeof(data));
    
    CHECK(result == true, "Send should succeed");
    CHECK(net.getOutgoingQueueSize() == 1, "One packet should be in outgoing queue");
    PASS();
}

void test_network_interface_receive_packet() {
    TEST("network_interface_receive_packet");
    MockNetworkInterface net;
    NetworkAddress src(0x7F000001, 10666);
    NetworkAddress dst(0x7F000001, 10667);
    uint8_t data[] = {0xAA, 0xBB, 0xCC, 0xDD};
    
    NetworkPacket incoming(src, dst, data, sizeof(data));
    net.queueIncomingPacket(incoming);
    
    NetworkPacket received;
    bool result = net.receivePacket(received);
    
    CHECK(result == true, "Receive should succeed");
    CHECK(received.source.ip == src.ip, "Source IP should match");
    CHECK(received.source.port == src.port, "Source port should match");
    CHECK(received.getByte(0) == 0xAA, "First byte should match");
    CHECK(received.getByte(3) == 0xDD, "Last byte should match");
    PASS();
}

void test_network_interface_no_incoming() {
    TEST("network_interface_no_incoming");
    MockNetworkInterface net;
    
    NetworkPacket received;
    bool result = net.receivePacket(received);
    
    CHECK(result == false, "Receive should fail when queue is empty");
    PASS();
}

void test_network_interface_multiple_packets() {
    TEST("network_interface_multiple_packets");
    MockNetworkInterface net;
    NetworkAddress src1(0x7F000001, 10666);
    NetworkAddress src2(0x7F000001, 10668);
    NetworkAddress dst(0x7F000001, 10667);
    
    uint8_t data1[] = {0x01};
    uint8_t data2[] = {0x02};
    
    net.queueIncomingPacket(NetworkPacket(src1, dst, data1, sizeof(data1)));
    net.queueIncomingPacket(NetworkPacket(src2, dst, data2, sizeof(data2)));
    
    NetworkPacket pkt1, pkt2;
    CHECK(net.receivePacket(pkt1) == true, "First receive should succeed");
    CHECK(net.receivePacket(pkt2) == true, "Second receive should succeed");
    CHECK(net.getIncomingQueueSize() == 0, "Queue should be empty after receiving all");
    PASS();
}

void test_network_interface_address_getters() {
    TEST("network_interface_address_getters");
    MockNetworkInterface net(0xC0A80001, 12345);  // 192.168.0.1:12345
    
    NetworkAddress local = net.getLocalAddress();
    CHECK(local.ip == 0xC0A80001, "Local IP should match");
    CHECK(local.port == 12345, "Local port should match");
    PASS();
}

void test_network_interface_packet_loss() {
    TEST("network_interface_packet_loss");
    MockNetworkInterface net;
    NetworkAddress dest(0x7F000001, 10667);
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    
    net.enablePacketLoss(1.0);  // 100% packet loss
    
    bool result = net.sendPacket(dest, data, sizeof(data));
    CHECK(result == false, "Send should fail with 100% packet loss");
    PASS();
}

void test_network_interface_packet_loss_disabled() {
    TEST("network_interface_packet_loss_disabled");
    MockNetworkInterface net;
    NetworkAddress dest(0x7F000001, 10667);
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    
    net.enablePacketLoss(1.0);
    net.disablePacketLoss();
    
    bool result = net.sendPacket(dest, data, sizeof(data));
    CHECK(result == true, "Send should succeed when packet loss disabled");
    PASS();
}

//============================================================================
// Packet Exchange Tests
//============================================================================

void test_packet_exchange_basic() {
    TEST("packet_exchange_basic");
    MockNetworkInterface client(0x7F000001, 10667);
    MockNetworkInterface server(0x7F000001, 10666);
    
    // Client sends connection request
    uint8_t request[] = {0x01, 0x00, 0x00, 0x00};  // packet type + reserved
    client.setRemoteAddress(server.getLocalAddress());
    client.sendPacket(server.getLocalAddress(), request, sizeof(request));
    
    // Server receives
    NetworkPacket received;
    bool rx = server.receivePacket(received);
    
    CHECK(rx == true, "Server should receive packet");
    CHECK(received.getByte(0) == 0x01, "Packet type should be 0x01");
    PASS();
}

void test_packet_exchange_response() {
    TEST("packet_exchange_response");
    MockNetworkInterface client(0x7F000001, 10667);
    MockNetworkInterface server(0x7F000001, 10666);
    
    // Client sends request
    uint8_t request[] = {0x01};
    client.sendPacket(server.getLocalAddress(), request, sizeof(request));
    
    // Server receives and sends response
    NetworkPacket req;
    server.receivePacket(req);
    
    uint8_t response[] = {0x02, 0x00};  // ack + result
    server.sendPacket(server.getLocalAddress(), response, sizeof(response));
    
    // Client receives response
    NetworkPacket resp;
    bool rx = client.receivePacket(resp);
    
    CHECK(rx == true, "Client should receive response");
    CHECK(resp.getByte(0) == 0x02, "Response type should be 0x02");
    PASS();
}

void test_packet_sequence_numbers() {
    TEST("packet_sequence_numbers");
    MockNetworkInterface net;
    NetworkAddress dest(0x7F000001, 10667);
    uint8_t data[] = {0xAA};
    
    net.sendPacket(dest, data, sizeof(data));
    net.sendPacket(dest, data, sizeof(data));
    net.sendPacket(dest, data, sizeof(data));
    
    CHECK(net.getOutgoingQueueSize() == 3, "Should have 3 packets queued");
    
    NetworkPacket pkt;
    net.receivePacket(pkt);
    CHECK(pkt.sequence_num == 0, "First packet should have sequence 0");
    
    net.receivePacket(pkt);
    CHECK(pkt.sequence_num == 1, "Second packet should have sequence 1");
    
    net.receivePacket(pkt);
    CHECK(pkt.sequence_num == 2, "Third packet should have sequence 2");
    PASS();
}

void test_packet_data_integrity() {
    TEST("packet_data_integrity");
    MockNetworkInterface net;
    NetworkAddress src(0x7F000001, 10666);
    NetworkAddress dst(0x7F000001, 10667);
    
    // Complex data pattern
    uint8_t complex_data[] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
    };
    
    net.queueIncomingPacket(NetworkPacket(src, dst, complex_data, sizeof(complex_data)));
    
    NetworkPacket received;
    net.receivePacket(received);
    
    // Verify all bytes
    for (size_t i = 0; i < sizeof(complex_data); i++) {
        CHECK(received.getByte(i) == complex_data[i], "Byte " + to_string(i) + " mismatch");
    }
    PASS();
}

void test_packet_empty_data() {
    TEST("packet_empty_data");
    MockNetworkInterface net;
    NetworkAddress src(0x7F000001, 10666);
    NetworkAddress dst(0x7F000001, 10667);
    
    uint8_t empty_data[] = {};
    net.queueIncomingPacket(NetworkPacket(src, dst, empty_data, 0));
    
    NetworkPacket received;
    bool result = net.receivePacket(received);
    
    CHECK(result == true, "Empty packet should be received");
    CHECK(received.getSize() == 0, "Packet size should be 0");
    PASS();
}

void test_packet_large_data() {
    TEST("packet_large_data");
    MockNetworkInterface net;
    NetworkAddress src(0x7F000001, 10666);
    NetworkAddress dst(0x7F000001, 10667);
    
    // Create 1024 byte packet
    std::vector<uint8_t> large_data(1024);
    for (size_t i = 0; i < 1024; i++) {
        large_data[i] = static_cast<uint8_t>(i & 0xFF);
    }
    
    net.queueIncomingPacket(NetworkPacket(src, dst, large_data.data(), large_data.size()));
    
    NetworkPacket received;
    bool result = net.receivePacket(received);
    
    CHECK(result == true, "Large packet should be received");
    CHECK(received.getSize() == 1024, "Packet size should be 1024");
    CHECK(received.getByte(0) == 0x00, "First byte should be 0x00");
    CHECK(received.getByte(512) == 0x00, "Middle byte should be 0x00");
    CHECK(received.getByte(1023) == 0xFF, "Last byte should be 0xFF");
    PASS();
}

void test_packet_address_preservation() {
    TEST("packet_address_preservation");
    MockNetworkInterface net;
    NetworkAddress src(0xC0A80001, 12345);  // 192.168.0.1:12345
    NetworkAddress dst(0xC0A80002, 54321);   // 192.168.0.2:54321
    
    uint8_t data[] = {0x01, 0x02, 0x03};
    net.queueIncomingPacket(NetworkPacket(src, dst, data, sizeof(data)));
    
    NetworkPacket received;
    net.receivePacket(received);
    
    CHECK(received.source.ip == src.ip, "Source IP should be preserved");
    CHECK(received.source.port == src.port, "Source port should be preserved");
    CHECK(received.destination.ip == dst.ip, "Destination IP should be preserved");
    CHECK(received.destination.port == dst.port, "Destination port should be preserved");
    PASS();
}

//============================================================================
// Packet Type Tests
//============================================================================

void test_packet_types_connection_request() {
    TEST("packet_types_connection_request");
    MockNetworkInterface net;
    NetworkAddress src(0x7F000001, 10667);
    NetworkAddress dst(0x7F000001, 10666);
    
    // Connection request packet
    uint8_t conn_req[] = {
        0x01,  // packet type: connection request
        0x00,  // reserved
        0x00, 0x00, 0x00, 0x01,  // protocol version
        0x44, 0x4F, 0x4F, 0x4D,  // "DOOM"
        0x4C, 0x45, 0x47, 0x41,  // "LEGA"
        0x43, 0x59              // "CY"
    };
    
    net.queueIncomingPacket(NetworkPacket(src, dst, conn_req, sizeof(conn_req)));
    
    NetworkPacket received;
    net.receivePacket(received);
    
    CHECK(received.getByte(0) == 0x01, "Packet type should be connection request");
    CHECK(received.getByte(4) == 0x01, "Protocol version should match");
    PASS();
}

void test_packet_types_connection_accept() {
    TEST("packet_types_connection_accept");
    MockNetworkInterface net;
    NetworkAddress src(0x7F000001, 10666);
    NetworkAddress dst(0x7F000001, 10667);
    
    // Connection accept packet
    uint8_t conn_accept[] = {
        0x02,  // packet type: connection accept
        0x00,  // reserved
        0x12, 0x34, 0x56, 0x78  // connection ID
    };
    
    net.queueIncomingPacket(NetworkPacket(src, dst, conn_accept, sizeof(conn_accept)));
    
    NetworkPacket received;
    net.receivePacket(received);
    
    CHECK(received.getByte(0) == 0x02, "Packet type should be connection accept");
    CHECK(received.getByte(2) == 0x34, "Connection ID byte 2 mismatch");
    PASS();
}

void test_packet_types_disconnect() {
    TEST("packet_types_disconnect");
    MockNetworkInterface net;
    NetworkAddress src(0x7F000001, 10666);
    NetworkAddress dst(0x7F000001, 10667);
    
    // Disconnect packet
    uint8_t disconnect[] = {
        0x05,  // packet type: disconnect
        0x01,  // reason: 1 = user requested
        'B', 'Y', 'E', '\0'  // message
    };
    
    net.queueIncomingPacket(NetworkPacket(src, dst, disconnect, sizeof(disconnect)));
    
    NetworkPacket received;
    net.receivePacket(received);
    
    CHECK(received.getByte(0) == 0x05, "Packet type should be disconnect");
    CHECK(received.getByte(1) == 0x01, "Reason should be user requested");
    PASS();
}

void test_packet_types_ping() {
    TEST("packet_types_ping");
    MockNetworkInterface net;
    NetworkAddress src(0x7F000001, 10666);
    NetworkAddress dst(0x7F000001, 10667);
    
    // Ping packet
    uint8_t ping[] = {
        0x06,  // packet type: ping
        0x00,  // sequence number
        0x00, 0x00, 0x00, 0x00  // timestamp
    };
    
    net.queueIncomingPacket(NetworkPacket(src, dst, ping, sizeof(ping)));
    
    NetworkPacket received;
    net.receivePacket(received);
    
    CHECK(received.getByte(0) == 0x06, "Packet type should be ping");
    PASS();
}

void test_packet_types_data() {
    TEST("packet_types_data");
    MockNetworkInterface net;
    NetworkAddress src(0x7F000001, 10667);
    NetworkAddress dst(0x7F000001, 10666);
    
    // Data packet (game state update)
    uint8_t data_pkt[] = {
        0x0A,  // packet type: data
        0x00,  // channel
        0x00, 0x00, 0x00, 0x01,  // sequence
        0x01, 0x02, 0x03  // payload
    };
    
    net.queueIncomingPacket(NetworkPacket(src, dst, data_pkt, sizeof(data_pkt)));
    
    NetworkPacket received;
    net.receivePacket(received);
    
    CHECK(received.getByte(0) == 0x0A, "Packet type should be data");
    CHECK(received.getByte(1) == 0x00, "Channel should be 0");
    CHECK(received.getByte(3) == 0x01, "Sequence byte 3 mismatch");
    PASS();
}

//============================================================================
// Main
//============================================================================

int main() {
    cout << "========================================" << endl;
    cout << "Network Integration Tests" << endl;
    cout << "========================================" << endl;
    
    cout << "\n[Connection Establishment Tests]" << endl;
    test_connection_disconnected_initial_state();
    test_connection_initiate();
    test_connection_accept();
    test_connection_disconnect();
    test_connection_double_connect_fails();
    test_connection_from_connected_fails();
    test_connection_accept_from_disconnected();
    test_connection_multiple_clients();
    test_connection_ping_measurement();
    
    cout << "\n[Network Interface Tests]" << endl;
    test_network_interface_initial_state();
    test_network_interface_send_packet();
    test_network_interface_receive_packet();
    test_network_interface_no_incoming();
    test_network_interface_multiple_packets();
    test_network_interface_address_getters();
    test_network_interface_packet_loss();
    test_network_interface_packet_loss_disabled();
    
    cout << "\n[Packet Exchange Tests]" << endl;
    test_packet_exchange_basic();
    test_packet_exchange_response();
    test_packet_sequence_numbers();
    test_packet_data_integrity();
    test_packet_empty_data();
    test_packet_large_data();
    test_packet_address_preservation();
    
    cout << "\n[Packet Type Tests]" << endl;
    test_packet_types_connection_request();
    test_packet_types_connection_accept();
    test_packet_types_disconnect();
    test_packet_types_ping();
    test_packet_types_data();
    
    cout << "\n========================================" << endl;
    cout << "Results: " << tests_passed << "/" << tests_run << " tests passed" << endl;
    cout << "========================================" << endl;
    
    return (tests_passed == tests_run) ? 0 : 1;
}
