let canvas, ctx;
const allIssues = new Set();
const realFonts = [];


document.addEventListener('DOMContentLoaded', async () => {
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

document.querySelectorAll('.subpage').forEach((subpage) => {
    observer.observe(subpage);
    // checkDesignIssues(subpage); // This will now check all child elements
});

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
    ctx.font = `16px "${fontName}", "monospace`;
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
