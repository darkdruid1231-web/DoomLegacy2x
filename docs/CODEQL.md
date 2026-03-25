# CodeQL Setup for Doom Legacy

## Two Branches, Two Databases

Doom Legacy has two active code branches:

| Path | Branch | Purpose |
|------|--------|---------|
| `legacy/trunk/` | 2.x (main) | Current development |
| `legacy/one/` | 1.x (legacy) | Legacy branch |

Each is a separate CodeQL database. AI agents can query both to compare logic across branches.

## Quick Start: Run CodeQL Locally

### 1. Install CodeQL CLI

```bash
# Download and extract CodeQL CLI
curl -sL https://github.com/github/codeql-cli-binaries/releases/latest/download/codeql-linux64.zip -o codeql.zip
unzip -q codeql.zip
export PATH=$PWD/codeql:$PATH
codeql version  # confirm it works
```

For Windows, use the Windows zip instead:
```bash
curl -sL https://github.com/github/codeql-cli-binaries/releases/latest/download/codeql-win64.zip -o codeql.zip
unzip -q codeql.zip
export PATH=$PWD/codeql:$PATH
```

### 2. Create a CodeQL Database (choose branch)

**For 2.x trunk:**
```bash
cd legacy/trunk
codeql database create doomlegacy-trunk-db --language=cpp --source-root=. \
  --command="cmake -G Ninja -B build-codeql -DCMAKE_BUILD_TYPE=Release .." \
  --command="ninja -C build-codeql"
```

**For 1.x legacy:**
```bash
cd legacy/one
codeql database create doomlegacy-one-db --language=cpp --source-root=. \
  --command="cmake -G Ninja -B build-codeql -DCMAKE_BUILD_TYPE=Release .." \
  --command="ninja -C build-codeql"
```

### 3. Run a Query

```bash
# List all functions in trunk
codeql query run "functions *" --database=doomlegacy-trunk-db --format=csv --output=trunk-funcs.csv

# Compare with 1.x
codeql query run "functions *" --database=doomlegacy-one-db --format=csv --output=one-funcs.csv
```

## Using the GitHub Actions Database Artifact

AI agents can download a pre-built database from GitHub Actions instead of building locally:

1. Go to: https://github.com/doomlegacydoom/doomlegacy/actions/workflows/codeql.yml
2. Find a recent successful run
3. Download the `codeql-db-cpp` artifact
4. Extract and query it:
```bash
tar -xzf codeql-db-cpp.tar.gz
codeql query run --database=codeql-db query-file.ql --format=csv --output=results.csv
```

## Sample Queries

### Compare actor spawn functions between branches

```ql
-- In trunk: find all P_Spawn* functions
import cpp
from Function f
where f.getName().regexpMatch("P_Spawn.*")
select f.getName(), f.getLocation()
```

### Find call paths from player movement to teleport (trunk)

```ql
import cpp
select * from ControlFlow::getAnomalies()
```

### Find functions that exist in trunk but NOT in legacy 1.x

Run the same function query on both databases, then diff the CSV outputs:
```bash
diff trunk-funcs.csv one-funcs.csv
```

### Find all direct memory accesses (potential buffer overflows)

```ql
import cpp
from Expr e
where e.getType().(ArrayType) or e.getType().(PointerType)
select e.getLocation(), e.getType().getName()
```

## Workflow Files

- `.github/workflows/codeql.yml` — builds `legacy/trunk` database on every push/PR to `main`
- `legacy/one` is not in the CI pipeline — create the database locally or extend the workflow
