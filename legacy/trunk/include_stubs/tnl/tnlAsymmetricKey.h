// Stub for tnl/tnlAsymmetricKey.h — TNL public-key crypto, not needed in stub builds.
#pragma once
namespace TNL {
class AsymmetricKey {
public:
    explicit AsymmetricKey(int /*keySize*/) {}
    ~AsymmetricKey() {}
    bool isValid() const { return false; }
};
} // namespace TNL
