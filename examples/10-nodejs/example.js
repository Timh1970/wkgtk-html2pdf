#!/usr/bin/env node
const path = require('path');

// Map locally or to standard global node-path distributions
const { PDFPrinter } = require('../../bindings/nodejs/wkgtkpdf.js');

print("[Node.js Example] Spawning isolated WebKitGTK print daemon thread...");
const printer = new PDFPrinter();

const html = "<html><body><h1>Node.js Print Run</h1><p>Concurrently rendering vector layouts via native C++.</p></body></html>";
const outPath = path.resolve('./node_print_demo.pdf');

printer.setParam(html, outPath);
printer.makePdf();

console.log(`[Node.js Example] Success! PDF compiled cleanly at: ${outPath}`);

