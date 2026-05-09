#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include "ssr2mihomo/fetcher.h"
#include "ssr2mihomo/mihomo.h"
#include "ssr2mihomo/ssr.h"
#include "ssr2mihomo/vmess.h"

namespace {

void expect(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void test_parse_subscription_decodes_ssr_nodes() {
    const std::string subscription =
        "c3NyOi8vWlhoaGJYQnNaUzVqYjIwNk9ETTRPRHBoZFhSb1gzTm9ZVEZmZGpRNllX"
        "VnpMVEkxTmkxalptSTZkR3h6TVM0eVgzUnBZMnRsZEY5aGRYUm9PbUpZYkhkWldF"
        "NTZMejl5WlcxaGNtdHpQVlpIVm5wa1EwSlBZakpTYkNadlltWnpjR0Z5WVcwOVds"
        "aG9hR0pZUW5OYVV6VnFZakl3Sm5CeWIzUnZjR0Z5WVcwOVRWUkplazl1UW1oak0w"
        "MApzc3I6Ly9jMlZ5ZG1WeU1pNWxlR0Z0Y0d4bE9qUTBNenB2Y21sbmFXNDZZMmho"
        "WTJoaE1qQXRhV1YwWmpwd2JHRnBianBqTWxacVkyMVdNQzhfYjJKbWMzQmhjbUZ0"
        "UFEK";

    const ssr2mihomo::ParseResult result = ssr2mihomo::parse_subscription(subscription);

    expect(result.nodes.size() == 2, "expected two parsed nodes");
    expect(result.warnings.empty(), "expected no warnings for valid sample");

    const auto& first = result.nodes.at(0);
    expect(first.name == "Test Node", "expected remarks to become node name");
    expect(first.server == "example.com", "expected first server");
    expect(first.port == 8388, "expected first port");
    expect(first.protocol == "auth_sha1_v4", "expected first protocol");
    expect(first.cipher == "aes-256-cfb", "expected first cipher");
    expect(first.obfs == "tls1.2_ticket_auth", "expected first obfs");
    expect(first.password == "mypass", "expected decoded password");
    expect(first.obfs_param == "example.com", "expected decoded obfs param");
    expect(first.protocol_param == "123:pass", "expected decoded protocol param");

    const auto& second = result.nodes.at(1);
    expect(second.name == "ssr-2", "expected fallback name for unnamed node");
    expect(second.server == "server2.example", "expected second server");
    expect(second.port == 443, "expected second port");
    expect(second.protocol == "origin", "expected second protocol");
    expect(second.cipher == "chacha20-ietf", "expected second cipher");
    expect(second.obfs == "plain", "expected second obfs");
    expect(second.password == "secret", "expected second password");
}

void test_render_mihomo_config_contains_expected_sections() {
    std::vector<ssr2mihomo::SsrNode> nodes = {
        {
            "Test Node",
            "example.com",
            8388,
            "aes-256-cfb",
            "mypass",
            "auth_sha1_v4",
            "123:pass",
            "tls1.2_ticket_auth",
            "example.com",
        },
        {
            "ssr-2",
            "server2.example",
            443,
            "chacha20-ietf",
            "secret",
            "origin",
            "",
            "plain",
            "",
        },
    };

    const std::string yaml =
        ssr2mihomo::render_mihomo_config(nodes, std::vector<ssr2mihomo::VmessNode>{}, "PROXY");

    expect(yaml.find("mixed-port: 7890") != std::string::npos, "expected mixed-port");
    expect(yaml.find("type: ssr") != std::string::npos, "expected ssr proxy type");
    expect(yaml.find("name: 'Test Node'") != std::string::npos, "expected first node name");
    expect(yaml.find("server: 'example.com'") != std::string::npos, "expected first server");
    expect(yaml.find("proxy-groups:") != std::string::npos, "expected proxy groups");
    expect(yaml.find("name: 'AUTO'") != std::string::npos, "expected auto group");
    expect(yaml.find("GEOIP,CN,DIRECT") != std::string::npos, "expected base rule");
    expect(yaml.find("MATCH,PROXY") != std::string::npos, "expected catch-all rule");
}

void test_choose_user_agent_uses_shadowrocket_when_requested() {
    const std::string shadowrocket_ua = ssr2mihomo::choose_user_agent(
        "https://example.com/sub?shadowrocket=1&extend=1", "");
    expect(shadowrocket_ua == "Shadowrocket/1.0",
           "expected shadowrocket links to use Shadowrocket UA");

    const std::string default_ua =
        ssr2mihomo::choose_user_agent("https://example.com/sub?id=1", "");
    expect(default_ua.find("Mozilla/5.0") != std::string::npos,
           "expected generic links to use browser-like UA");

    const std::string custom_ua = ssr2mihomo::choose_user_agent(
        "https://example.com/sub?shadowrocket=1", "CustomAgent/9.9");
    expect(custom_ua == "CustomAgent/9.9", "expected explicit UA override");
}

void test_parse_subscription_ignores_metadata_and_marks_unsupported_scheme() {
    const std::string subscription =
        "U1RBVFVTPVJlbWFpbmluZzogMUdCClJFTUFSS1M9RGVtbwpzc3I6Ly9aWGhoYlhCc1pTNWpiMjA2"
        "T0RNNE9EcGhkWFJvWDNOb1lURmZkalE2WVdWekxUSTFOaTFqWm1JNmRHeHpNUzR5WDNScFkydGxk"
        "RjloZFhSb09tSlliSGRaV0U1Nkx6OXlaVzFoY210elBWWkhWbnBrUTBKUFlqSlNiQ1p2WW1aemNH"
        "RnlZVzA5V2xob2FHSllRbk5hVXpWcVlqSXdKbkJ5YjNSdmNHRnlZVzA5VFZSSmVrOXVRbWhqTTAw"
        "CnZsZXNzOi8vYWJjMTIzCg==";

    const ssr2mihomo::ParseResult result = ssr2mihomo::parse_subscription(subscription);

    expect(result.nodes.size() == 1, "expected metadata lines to be ignored");
    expect(result.nodes.at(0).name == "Test Node", "expected SSR node to still parse");
    expect(result.vmess_nodes.empty(), "expected no vmess nodes");
    expect(result.warnings.size() == 1, "expected one warning for unsupported vless");
    expect(result.warnings.at(0).find("unsupported subscription entry") != std::string::npos,
           "expected unsupported scheme warning");
}

void test_parse_vmess_uri_decodes_json_payload() {
    // Synthetic V2RayN-style fixture: base64 of
    // {"v":"2","ps":"Test VMess","add":"vmess.example","port":"443",
    //  "id":"11111111-1111-1111-1111-111111111111","aid":"0","scy":"auto","net":"ws",
    //  "type":"none","host":"host.example","path":"/ws","tls":"tls","sni":"sni.example"}
    const std::string uri =
        "vmess://eyJ2IjoiMiIsInBzIjoiVGVzdCBWTWVzcyIsImFkZCI6InZtZXNzLmV4YW1wbGUiLCJwb3J0"
        "IjoiNDQzIiwiaWQiOiIxMTExMTExMS0xMTExLTExMTEtMTExMS0xMTExMTExMTExMTEiLCJhaWQiOiIw"
        "Iiwic2N5IjoiYXV0byIsIm5ldCI6IndzIiwidHlwZSI6Im5vbmUiLCJob3N0IjoiaG9zdC5leGFtcGxl"
        "IiwicGF0aCI6Ii93cyIsInRscyI6InRscyIsInNuaSI6InNuaS5leGFtcGxlIn0=";

    const ssr2mihomo::VmessNode node = ssr2mihomo::parse_vmess_uri(uri);

    expect(node.name == "Test VMess", "expected ps as node name");
    expect(node.server == "vmess.example", "expected add as server");
    expect(node.port == 443, "expected port 443");
    expect(node.uuid == "11111111-1111-1111-1111-111111111111", "expected uuid");
    expect(node.alter_id == 0, "expected aid 0");
    expect(node.cipher == "auto", "expected scy=auto");
    expect(node.network == "ws", "expected network=ws");
    expect(node.tls, "expected tls=true");
    expect(node.servername == "sni.example", "expected sni");
    expect(node.host == "host.example", "expected host");
    expect(node.path == "/ws", "expected path");
}

void test_parse_vmess_uri_handles_shadowrocket_format() {
    // Synthetic Shadowrocket-style fixture: base64 of
    //   chacha20-poly1305:22222222-2222-2222-2222-222222222222@198.51.100.7:12345
    // (198.51.100.0/24 is RFC 5737 TEST-NET-2)
    const std::string uri =
        "vmess://Y2hhY2hhMjAtcG9seTEzMDU6MjIyMjIyMjItMjIyMi0yMjIyLTIyMjItMjIyMjIyMjIyMjIy"
        "QDE5OC41MS4xMDAuNzoxMjM0NQ"
        "?remarks=Test%20Node"
        "&obfsParam=host.example"
        "&path=/ws"
        "&obfs=websocket"
        "&tls=0";

    const ssr2mihomo::VmessNode node = ssr2mihomo::parse_vmess_uri(uri);

    expect(node.cipher == "chacha20-poly1305", "expected method as cipher");
    expect(node.uuid == "22222222-2222-2222-2222-222222222222", "expected uuid");
    expect(node.server == "198.51.100.7", "expected server");
    expect(node.port == 12345, "expected port");
    expect(node.name == "Test Node", "expected url-decoded remarks");
    expect(node.network == "ws", "expected ws network from obfs=websocket");
    expect(node.path == "/ws", "expected path");
    expect(node.host == "host.example", "expected obfsParam as host");
    expect(!node.tls, "expected tls=false from tls=0");
}

void test_parse_subscription_collects_mixed_ssr_and_vmess() {
    // Outer base64 of two lines:
    //   ssr://...  (the same Test-Node fixture used elsewhere)
    //   vmess://<base64 of the synthetic V2RayN JSON above>
    const std::string subscription =
        "c3NyOi8vWlhoaGJYQnNaUzVqYjIwNk9ETTRPRHBoZFhSb1gzTm9ZVEZmZGpRNllXVnpMVEkxTmkxalpt"
        "STZkR3h6TVM0eVgzUnBZMnRsZEY5aGRYUm9PbUpZYkhkWldFNTZMejl5WlcxaGNtdHpQVlpIVm5wa1Ew"
        "SlBZakpTYkNadlltWnpjR0Z5WVcwOVdsaG9hR0pZUW5OYVV6VnFZakl3Sm5CeWIzUnZjR0Z5WVcwOVRW"
        "Ukplazl1UW1oak0wMAp2bWVzczovL2V5SjJJam9pTWlJc0luQnpJam9pVkdWemRDQldUV1Z6Y3lJc0lt"
        "RmtaQ0k2SW5adFpYTnpMbVY0WVcxd2JHVWlMQ0p3YjNKMElqb2lORFF6SWl3aWFXUWlPaUl4TVRFeE1U"
        "RXhNUzB4TVRFeExURXhNVEV0TVRFeE1TMHhNVEV4TVRFeE1URXhNVEVpTENKaGFXUWlPaUl3SWl3aWMy"
        "TjVJam9pWVhWMGJ5SXNJbTVsZENJNkluZHpJaXdpZEhsd1pTSTZJbTV2Ym1VaUxDSm9iM04wSWpvaWFH"
        "OXpkQzVsZUdGdGNHeGxJaXdpY0dGMGFDSTZJaTkzY3lJc0luUnNjeUk2SW5Sc2N5SXNJbk51YVNJNklu"
        "TnVhUzVsZUdGdGNHeGxJbjA9Cg==";

    const ssr2mihomo::ParseResult result = ssr2mihomo::parse_subscription(subscription);

    expect(result.nodes.size() == 1, "expected one SSR node");
    expect(result.vmess_nodes.size() == 1, "expected one vmess node");
    expect(result.warnings.empty(), "expected no warnings");
    expect(result.vmess_nodes.at(0).name == "Test VMess", "expected vmess ps");
    expect(result.vmess_nodes.at(0).server == "vmess.example", "expected vmess server");
    expect(result.vmess_nodes.at(0).port == 443, "expected vmess port");
}

void test_render_mihomo_config_emits_vmess_block() {
    std::vector<ssr2mihomo::SsrNode> ssr_nodes;
    std::vector<ssr2mihomo::VmessNode> vmess_nodes = {
        {
            "VMess WS",
            "vmess.example",
            443,
            "11111111-1111-1111-1111-111111111111",
            0,
            "auto",
            "ws",
            true,
            "sni.example",
            "host.example",
            "/ws",
            "",
            "",
        },
    };

    const std::string yaml =
        ssr2mihomo::render_mihomo_config(ssr_nodes, vmess_nodes, "PROXY");

    expect(yaml.find("type: vmess") != std::string::npos, "expected vmess proxy type");
    expect(yaml.find("uuid: '11111111-1111-1111-1111-111111111111'") != std::string::npos,
           "expected uuid line");
    expect(yaml.find("alterId: 0") != std::string::npos, "expected alterId");
    expect(yaml.find("network: 'ws'") != std::string::npos, "expected network ws");
    expect(yaml.find("ws-opts:") != std::string::npos, "expected ws-opts block");
    expect(yaml.find("path: '/ws'") != std::string::npos, "expected ws path");
    expect(yaml.find("Host: 'host.example'") != std::string::npos, "expected ws Host header");
    expect(yaml.find("servername: 'sni.example'") != std::string::npos, "expected SNI");
    expect(yaml.find("- 'VMess WS'") != std::string::npos,
           "expected vmess name in proxy-groups");
}

}  // namespace

int main() {
    try {
        test_parse_subscription_decodes_ssr_nodes();
        test_render_mihomo_config_contains_expected_sections();
        test_choose_user_agent_uses_shadowrocket_when_requested();
        test_parse_subscription_ignores_metadata_and_marks_unsupported_scheme();
        test_parse_vmess_uri_decodes_json_payload();
        test_parse_vmess_uri_handles_shadowrocket_format();
        test_parse_subscription_collects_mixed_ssr_and_vmess();
        test_render_mihomo_config_emits_vmess_block();
    } catch (const std::exception& ex) {
        std::cerr << "test failure: " << ex.what() << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "all tests passed\n";
    return EXIT_SUCCESS;
}
