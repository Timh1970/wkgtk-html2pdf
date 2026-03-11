let canvas, ctx;
const allIssues = new Set();
const realFonts = [];

/**
 * Text-Surgical Engine Compensation
 * Finds all elements with direct text and applies a coefficient to their line-height.
 */
async function applyTextShave() {
    const ua = navigator.userAgent;

    // 1. Determine Coefficient (The "Magic Number")
    let coef = coefficient();

    if (coef === 1.0) return;

    // 2. Find all elements that actually contain text
    // We use a TreeWalker to find Text Nodes, then target their parent elements
    const walker = document.createTreeWalker(
        document.body,
        NodeFilter.SHOW_TEXT,
        null,
        false
    );

    const textElements = new Set();
    while(walker.nextNode()) {
        const parent = walker.currentNode.parentElement;
        // Ignore whitespace-only nodes and script/style tags
        if (walker.currentNode.textContent.trim().length > 0) {
            textElements.add(parent);
        }
    }

    // 3. Apply the Coefficient to the computed pixel value
    textElements.forEach(el => {
        const style = window.getComputedStyle(el);


        // 1. Adjust Font Size (The "Footprint")
        const currentFS = parseFloat(style.fontSize);
        el.style.fontSize = `${(currentFS * fsCoef).toFixed(4)}px`;

        // 2. Contract the Kerning
        const currentLS = (style.letterSpacing === 'normal') ? 0 : parseFloat(style.letterSpacing);
        el.style.letterSpacing = `${(currentLS + lsNudge).toPrecision(6)}px`;
        // 3. Line height
        const lh = style.lineHeight;

        let numericLH;
        if (lh === 'normal') {
            numericLH = parseFloat(style.fontSize) * 1.2;
        } else {
            numericLH = parseFloat(lh);
        }

        // Apply the coefficient and force it in pixels
        // This bypasses the engine's internal "normal" rounding logic
        const adjustedLH = (numericLH * coef).toPrecision(5);
        el.style.lineHeight = `${adjustedLH}px`;
    });

    console.log(`📏 Shaved ${textElements.size} text elements by ${coef}`);
};



async function initDesignHelper()  {
    console.log("🛠️ Starting Design Helper...");

    // 1. Wait for fonts (critical for accurate height measurements)
    await document.fonts.ready;

    // 2. Apply the Shave to all text-containing elements
    // This rewrites the Computed Style to match WebKitGTK metrics
    await applyTextShave();

    // 3. Brief "settle" time for the browser to recalculate layout
    await new Promise(resolve => requestAnimationFrame(resolve));

    // 4. Initialize the ResizeObserver now that the "shaved" reality is set
    document.querySelectorAll('.subpage').forEach((subpage) => {
        observer.observe(subpage);
    });
    //
    console.log("✅ Observer active on corrected metrics.");
};
var lsNudge = 0;
var fsCoef = 1.0;
// --- Engine Compensation Helper ---
function coefficient() {
        const ua = navigator.userAgent;
        console.log(`User Agent is ${ua}`);
        // 1. WebKit/WebKitGTK - THE BASELINE
        // If it's WebKit but NOT Chrome/OPR, it's our "Gold Standard"
        if (/AppleWebKit/.test(ua) && !/Chrome/.test(ua) && !/OPR/.test(ua)) {
            console.log("WebKit Browser detected");
            return 1.0;
        }
        // 2. Opera - The Chromium Pioneer
        if (/OPR/.test(ua) || /Opera/.test(ua)) {
            console.log("Opera Browser detected");
            return 0.987; // Opera's specific "magic number"
        }
        // 3. Falkon / QtWebEngine
        if (/QtWebEngine/.test(ua) || /Falkon/.test(ua)) {
            console.log("Falkon or other QtWebEngine Browser detected");
            fsCoef = 1.077;
            lsNudge = 1.0;
            return 0.89; // Falkon usually needs a lighter touch
        }
        // 4. Firefox (Gecko)
        if (/Firefox/.test(ua)) {
            console.log("Firefox Browser detected");
            return 1; // Gecko drift is minimal
        }
        // 5. Generic Chromium (Chrome, Edge, Brave)
        if (/Chrome/.test(ua)) {
            console.log("Chrome Browser detected");
            const version = parseInt(ua.match(/Chrome\/(\d+)/)?.[1] || 0);
            return version >= 140 ? 0.972 : 0.988;
        }
        return 1.0; // Fallback for anything else
}




document.addEventListener('DOMContentLoaded', async () => {

    initDesignHelper();




    canvas = document.createElement('canvas');
    canvas.style.display = 'none';
    canvas.width = 500;
    canvas.height = 300;
    document.body.appendChild(canvas);
    ctx = canvas.getContext('2d', {
        willReadFrequently: true
    });
    await logAllFonts();
});

const allFonts = new Set();

function sanitizeFont(fontFamily) {
    return fontFamily
        .replace(/['"]/g, '') // Remove quotes
        .split(',')[0] // Take first font
        .trim(); // Remove whitespace
}

const logAllFonts = () => {
    // Clear previous list
    allFonts.clear();

    // Get all elements with computed font-family
    document.querySelectorAll('*').forEach(element => {
        try {
            const computed = window.getComputedStyle(element);
            const fontFamily = computed.fontFamily;
            const cleanFont = sanitizeFont(fontFamily);
            allFonts.add(cleanFont);
        } catch (e) {
            // Ignore errors
        }
    });

    // Check @font-face rules
    const styleSheets = Array.from(document.styleSheets);
    styleSheets.forEach(sheet => {
        try {
            const rules = Array.from(sheet.cssRules || []);
            rules.forEach(rule => {
                if (rule.type === CSSRule.FONT_FACE_RULE) {
                    const fontName = rule.style.fontFamily;
                    const cleanFont = sanitizeFont(fontName);
                    allFonts.add(cleanFont);
                }
            });
        } catch (e) {
            // Ignore cross-origin errors
        }
    });

    console.log('=== ALL FONTS USED ON PAGE ===');
    console.log(Array.from(allFonts));
    console.log('==============================');
};

const checkDesignIssues = async (element) => {
    const issues = [];

    // Check overflow on the element itself
    if (element.scrollHeight > element.clientHeight || element.scrollWidth > element.clientWidth) {
        issues.push('OVERFLOW');
        element.style.border = "2px solid red";
    }

    return issues;
};

const observer = new ResizeObserver(async (entries) => {

    allIssues.clear();

    requestAnimationFrame(async () => {
        // Wait for all font checks to complete
        for (const entry of entries) {
            const issues = await checkDesignIssues(entry.target);
            issues.forEach(issue => allIssues.add(issue));
        }

        for (const f of allFonts) {
            const lowerFont = f.toLowerCase();
            if (f === "monospace") {
                console.log(`Test font loading: ${f} (skipping)`);
                continue;
            }
            if (await isFontRendered(f)) {
                realFonts.push(f);
            } else {
                console.log(`Missing font: ${f}`);
                allIssues.add(`FONT_MISSING: ${f}`);
            }
        }


        for (const item of realFonts) {
            console.log(item);
        }
        updateStatusPanel(Array.from(allIssues));
    });
});

// document.querySelectorAll('.subpage').forEach((subpage) => {
//     observer.observe(subpage);
//     // checkDesignIssues(subpage); // This will now check all child elements
// });

const createDesignStatusPanel = () => {
    const panel = document.createElement('div');
    panel.id = 'design-status-panel';
    panel.className = 'design-helper';
    panel.style.cssText = `
    position: fixed;
    bottom: 10px;
    right: 10px;
    background: #f0f0f0;
    border: 1px solid #ccc;
    padding: 10px;
    border-radius: 4px;
    font-family: monospace;
    font-size: 12px;
    max-width: 300px;
    max-height: 200px;
    overflow-y: auto;
    z-index: 10000;
    display: none;
  `;
    document.body.appendChild(panel);
    return panel;
};

const updateStatusPanel = (allIssues) => {
    let panel = document.getElementById('design-status-panel');
    if (!panel) panel = createDesignStatusPanel();

    if (allIssues.length === 0) {
        panel.style.display = 'none';
        return;
    }

    panel.style.display = 'block';
    panel.style.borderColor = allIssues.some(i => i.includes('OVERFLOW')) ? 'red' : 'brown';
    panel.innerHTML = `
    <strong style="color: #333;">Design Issues:</strong><br>
    ${allIssues.map(issue => `<div style="color: ${issue.includes('OVERFLOW') ? 'red' : 'brown'};">• ${issue}</div>`).join('')}
  `;

    document.querySelectorAll('.subpage').forEach((subpage) => {
        subpage.style.borderColor = allIssues.some(i => i.includes('OVERFLOW')) ? 'red' : 'brown';
    });

};

async function isFontRendered(fontName) {
    const testString = "ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789Il1!0OQ";
    await document.fonts.ready;
    while (!ctx) await new Promise(r => setTimeout(r, 10));

    // Draw with test font + fallback
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    ctx.font = `16px "${fontName}", "monospace"`;
    ctx.fillText(testString, 10, 50);
    const withFont = ctx.getImageData(0, 0, canvas.width, canvas.height).data;

    // Draw with fallback only
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    ctx.font = `16px "monospace"`;

    ctx.fillText(testString, 10, 50);
    const withFallback = ctx.getImageData(0, 0, canvas.width, canvas.height).data;

    // If pixel data is the same → font didn't load
    return !pixelsEqual(withFont, withFallback);
}

function pixelsEqual(a, b) {
    for (let i = 0; i < a.length; i++)
        if (a[i] !== b[i]) return false;
    return true;
}
