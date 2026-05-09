# SSR2Mihomo

Download an SSR subscription URL, decode the `ssr://` entries inside it, and
generate a mihomo-compatible `config.yaml`.

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Test

```bash
cd build
ctest --output-on-failure
```

## Usage

```bash
./build/SSR2Mihomo \
  --url "https://subscription-url" \
  --output config.yaml
```

Optional flags:

- `--group PROXY` customizes the main mihomo proxy group name
- `--user-agent "Shadowrocket/1.0"` overrides the default request `User-Agent`
- `--allow-insecure` disables HTTPS certificate verification

The generated file includes:

- `proxies` for parsed SSR nodes
- `proxy-groups` with `PROXY`, `AUTO`, and `DIRECT`
- basic `rules` for `GEOIP,CN,DIRECT` and `MATCH,PROXY`
