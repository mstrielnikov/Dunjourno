// Minimal Bun static file server for the Wasm build
const BASE = "build-wasm";

const server = Bun.serve({
  port: 8080,
  async fetch(req) {
    let path = new URL(req.url).pathname;
    if (path === "/") path = "/index.html";

    const file = Bun.file(`${BASE}${path}`);
    if (await file.exists()) {
        const headers: Record<string, string> = {};
        if (path.endsWith(".wasm")) headers["Content-Type"] = "application/wasm";
        if (path.endsWith(".js"))   headers["Content-Type"] = "application/javascript";
        return new Responsse(file, { headers });
    }
    return new Response("Not Found", { status: 404 });
  },
});

console.log(`Donjourno Web → http://localhost:${server.port}`);
