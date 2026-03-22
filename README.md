# Planka MCP Server 🚀
> The most powerful, decentralized bridge between AI Agents and Planka v2.

[![Language](https://img.shields.io/badge/Language-C%2B%2B20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![License](https://img.shields.io/badge/License-Fair%20Use-green.svg)](https://github.com/plankanban/planka/blob/master/LICENSE.md)

This high-performance MCP server transforms your Planka instance into a structured knowledge base for AI Assistants (Claude, Cursor, etc.). 

## 🌟 Key Features

- **Progressive Discovery**: Instead of overwhelming your AI with 60+ tools, we use **Dynamic Resources** (`planka://hub`, `planka://projects`) to guide the Agent. It only sees what it needs, when it needs it.
- **The "God Hub"**: Start with `planka://hub` for a global overview of your entire Planka instance—projects, users, and unread notifications at a glance.
- **Decentralized Notifications**: Custom `Notification Services` support. Non-admin users can now build their own automation flows by bridging Planka events to local receivers via reverse-proxy (frp).
- **Stateless & Secure**: Zero persistence on the server side. Credentials are injected per-request by the Agent, ensuring absolute privacy and multi-tenant support.
- **Native C++20 Performance**: Built with `coke` and `wfrest`, leveraging high-concurrency coroutines for zero-latency interactions.
- **Archive-Aware**: Each board exposes `archiveListId` in its summary. To archive a card, move it to that list — `isArchived`/`isClosed` do nothing on cards and should not be used.

## 🛠 Available Tools

This MCP server provides a streamlined set of 5 core tools. It follows a **Resource-First** philosophy, where most tools interact with Planka via URI-based discovery.

| Tool | Description |
| :--- | :--- |
| `planka_explore` | **Core discovery tool.** Used to explore Planka resources via URIs (e.g., `planka://hub`, `planka://projects`). This acts as a compatibility bridge for clients that do not support native MCP Resources. |
| `planka_create` | Create any Planka entity: projects, boards, lists, cards, task lists, tasks, labels, comments, webhooks, and more. |
| `planka_update` | Update attributes of existing entities (e.g., card descriptions, board names, or moving a card to another list). |
| `planka_delete` | Securely remove entities from the system. |
| `planka_action` | Specialized operations: duplicating cards, moving all cards in a list, managing board/card members and labels. |

---

Planka-MCP adapts to your environment through three primary operation modes:

1.  **💻 Local Binary (Stdio)**: Direct integration with desktop AI clients (Cursor, Claude Desktop).
2.  **🌐 Background Service (HTTP/SSE)**: Host as a persistent web service (supports Systemd).
3.  **🐳 Containerized Gateway (Docker)**: Production-grade, multi-platform deployment.

---

## 🚀 Installation & Quick Start

### Option A: From Docker Hub (Recommended)
The easiest way to get started. No local build required.
```bash
docker pull tenire/planka-mcp:latest
```

**Run as a Stateless HTTP/SSE Gateway:**
```bash
# Recommended for IPv6 stability and proper signal handling
docker run -d --name planka-mcp \
  --network host \
  --init \
  --restart always \
  tenire/planka-mcp:latest --http --port 7526
```

---

### Option B: Build from Source
**Requirement:** Linux (Ubuntu 24.04), C++20 compiler.

1. **Install xmake**:
   ```bash
   curl -fsSL https://xmake.io/shget.text | bash
   ```

2. **Clone & Build**:
   ```bash
   git clone https://github.com/Tenicrab/planka-mcp.git
   cd planka-mcp
   xmake f -m release --yes
   xmake build planka-mcp
   xmake install -o ./dist planka-mcp
   ```
   Binary location: `./dist/bin/planka-mcp`.

**Run as a persistent HTTP/SSE Service:**
```bash
./dist/bin/planka-mcp --http --port 7526
```
(You can also use `systemd` to manage it as a background service).

### Option C: Manual Docker Build
If you want to build your own image locally:
```bash
docker build -t planka-mcp:latest -f deploy/Dockerfile .
```

---

## ⚙️ Configuration (mcpServers.json)

### 1. Stdio Mode (CLI Driven)
Ideal for **Claude Desktop** or **Cursor**. Credential injection is handled via arguments.

```json
{
  "mcpServers": {
    "planka": {
      "command": "/usr/local/bin/planka-mcp",
      "args": [
        "--stdio",
        "--url", "https://your-planka-instance.top:8080",
        "--api-key", "HwQsvOKO_YOUR_SECRET_KEY"
      ]
    }
  }
}
```

### 2. SSE Mode (Gateway Driven)
Ideal for **Docker** or cloud deployment. The server acts as a stateless bridge.

```json
{
  "mcpServers": {
    "planka": {
      "url": "http://your-gateway-ip:7526/",
      "headers": {
        "X-Planka-Url": "https://your-planka-instance.top:8080",
        "X-Planka-Api-Key": "HwQsvOKO_YOUR_SECRET_KEY"
      }
    }
  }
}
```

> or  
> "url": "http://your-gateway-ip:7526/mcp"
> "url": "http://your-gateway-ip:7526/sse"

---

## 🔒 Stateless Security

- **Agent-Driven**: This server follows the **Principle of Agent-Controlled Configuration**. It does not persist credentials.
- **Multitenancy**: A single server instance can safely serve multiple users with different Planka backends by using separate headers for each client connection.
- **Precision**: Enforces string-based Snowflake ID handling to prevent precision loss in LLM interactions.

## ⚠️ Networking Note
When running in Docker and forwarding events back to a local Agent, use `http://host.docker.internal:PORT` to bridge container isolation.
