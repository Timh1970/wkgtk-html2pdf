let canvas, ctx;
const allIssues = new Set();
const realFonts = [];

async function applyTextShave() {
    const ua = navigator.userAgent;
    let browserTest = coefficient();


    if (browserTest.coef === 1.0) {
        if (browserTest.msg != null) {
            allIssues.add(browserTest.msg);
        }
        return
    }

    const walker = document.createTreeWalker(
        document.body,
        NodeFilter.SHOW_TEXT,
        null,
        false
    );
    const textElements = new Set();
    while (walker.nextNode()) {
        const parent = walker.currentNode.parentElement;
        if (walker.currentNode.textContent.trim().length > 0) {
            textElements.add(parent);
        }
    }

    textElements.forEach(el => {
        const style = window.getComputedStyle(el);
        const currentFS = parseFloat(style.fontSize);
        el.style.fontSize = `${(currentFS * browserTest.fsCoef).toFixed(4)}pt`;

        const currentLS = (style.letterSpacing === 'normal') ? 0 : parseFloat(style.letterSpacing);
        el.style.letterSpacing = `${(currentLS + browserTest.lsNudge).toPrecision(6)}pt`;
        // 3. Line height
        const lh = style.lineHeight;

        let numericLH;
        if (lh === 'normal') {
            numericLH = parseFloat(style.fontSize) * 1.2;
        } else {
            numericLH = parseFloat(lh);
        }
        const adjustedLH = (numericLH * browserTest.coef).toPrecision(5);
        el.style.lineHeight = `${adjustedLH}pt`;
    });

    if (browserTest.msg != null) {
        allIssues.add(browserTest.msg);
    }

    console.log(`📏 Shaved ${textElements.size} text elements by ${browserTest.coef}`);
};



async function initDesignHelper() {
    console.log("🛠️ Starting Design Helper...");
    await document.fonts.ready;
    await applyTextShave();
    await new Promise(resolve => requestAnimationFrame(resolve));
    document.querySelectorAll('.subpage').forEach((subpage) => {
        observer.observe(subpage);
    });
    //
    console.log("✅ Observer active on corrected metrics.");
};


function coefficient() {
    // NOTE: THIS WAS USED FOR TESTING AND POTENTIAL CORRECTIONS PRIOR TO FULLY ALIGNING
    // THE TEMPLATES; IT IS NO LONGER NEEDED AS ALL BROWSERS ARE CURRENTLY COMPATIBLE,
    // BUT HAS BEEN LEFT IN SHOULD ANY FUTURE ISSUES DEVELOP.
    //
    // TO DISPLAY A MESSAGE ABOUT A SPECIFIC BROWSER SIMPLY TYPE THE word
    //
    // BROWSER:
    //
    // FOLLOWED BY YOUR MESSAGE IN THE RESPECTIVE STRUCT UNDER msg:
    //
    // eg.
    //
    // msg: "BROWSER: foo causes bar."
    //
    // CURRENTLY ALL MESSGES ARE SET TO null
    const ua = navigator.userAgent;
    console.log(`User Agent is ${ua}`);
    // 1. WebKit/WebKitGTK - THE BASELINE
    // If it's WebKit but NOT Chrome/OPR, it's our "Gold Standard"
    if (/AppleWebKit/.test(ua) && !/Chrome/.test(ua) && !/OPR/.test(ua)) {
        console.log("WebKit Browser detected");
        return {
            lsNudge: 0,
            fsCoef: 1.0,
            coef: 1.0,
            msg: null
        };
    }
    // 2. Opera - The Chromium Pioneer
    if (/OPR/.test(ua) || /Opera/.test(ua)) {
        return {
            lsNudge: 0,
            fsCoef: 1.0,
            coef: 1.0,
            msg: null
        };
    }
    // 3. Falkon / QtWebEngine
    if (/QtWebEngine/.test(ua) || /Falkon/.test(ua)) {
        return {
            lsNudge: 0,
            fsCoef: 1.0,
            coef: 1,
            msg: null
        };
    }
    // 4. Firefox (Gecko)
    if (/Firefox/.test(ua)) {
        return {
            lsNudge: 0,
            fsCoef: 1.0,
            coef: 1.0,
            msg: null
        };
    }
    // 5. Generic Chromium (Chrome, Edge, Brave)
    if (/Chrome/.test(ua)) {
        conlole.log("Chrome detected")
        const version = parseInt(ua.match(/Chrome\/(\d+)/)?.[1] || 0);
        let res = (version >= 140 ? 0.972 : 0.988);
        return {
            lsNudge: 0,
            fsCoef: 1.0,
            coef: 1.0,
            msg: null
        };
    }
    return {
        lsNudge: 0,
        fsCoef: 1.0,
        coef: 1.0,
        msg: null
    };

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

    // auditUnits();
});

const allFonts = new Set();

function sanitizeFont(fontFamily) {
    return fontFamily
        .replace(/['"]/g, '') // Remove quotes
        .split(',')[0] // Take first font
        .trim(); // Remove whitespace
}

const logAllFonts = () => {
    allFonts.clear();
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
        element.style.border = "3pt solid red";
    }

    return issues;
};

const observer = new ResizeObserver(async (entries) => {

    var i = allIssues.length;
    while (i--) {
        if (allIssues[i].indexOf('BROWSER:') < 0) {
            //-- splice will remove the non-matching element
            allIssues.splice(i, 1);
        }
    }


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

        const scanForLiarUnits = () => {
            const dodgySelectors = [];
            Array.from(document.styleSheets).forEach(sheet => {
                // 1. SKIP GHOSTS: Only check sheets that are local/inline or part of your templates
                // Browser internal sheets usually have a null or "chrome://" href
                if (sheet.href && !sheet.href.includes(window.location.hostname) && !sheet.href.startsWith('file:')) {
                    return;
                }

                try {
                    Array.from(sheet.cssRules).forEach(rule => {
                        const text = rule.cssText.toLowerCase();

                        // 2. TARGETED REGEX: Only flag if 'px' is actually used in the value
                        // This prevents flagging a selector name that happens to contain 'px'
                        if (/:\s*[1-9]\d*\.?\d*px|:\s*0\.\d*[1-9]\px/.test(text)) {
                            // 3. IGNORE LIST: Skip the helper and the page/subpage setup
                            const ignore = ['design-helper', '.page', '.subpage'];
                            if (!ignore.some(term => rule.selectorText?.includes(term))) {
                                dodgySelectors.push(`PX_UNIT: ${rule.selectorText}`);
                            }
                        }
                        if (/:\s*[1-9]\d*\.?\d*em|:\s*0\.\d*[1-9]\em/.test(text)) {
                            // 3. IGNORE LIST: Skip the helper and the page/subpage setup
                            const ignore = ['design-helper', '.page', '.subpage'];
                            if (!ignore.some(term => rule.selectorText?.includes(term))) {
                                dodgySelectors.push(`EM_UNIT: ${rule.selectorText}`);
                            }
                        }
                        if (/:\s*\d+\.(?!(0|25|5|75)0*\s*pt)\d+pt/.test(text)) {
                            const ignore = ['design-helper', '.page', '.subpage'];
                            if (!ignore.some(term => rule.selectorText?.includes(term))) {
                                dodgySelectors.push(`DIRTY_PRECISION: ${rule.selectorText}`);
                            }
                        }

                    });
                } catch (e) {}
            });
            return [...new Set(dodgySelectors)];
        };

        const scanForInlineLiarUnits = () => {
            const inlineDodgy = [];

            // First, clear any old snitch classes so we start fresh
            document.querySelectorAll('.design-helper-inline-liar')
                .forEach(el => el.classList.remove('design-helper-inline-liar'));

            const elementsWithStyle = document.querySelectorAll('[style]');

            elementsWithStyle.forEach(el => {
                const styleAttr = el.getAttribute('style').toLowerCase();

                // Use your "Non-Zero" regex
                if (/:\s*[1-9]\d*\.?\d*px|:\s*0\.\d*[1-9]px/.test(styleAttr)) {
                    const identifier = el.id ? `#${el.id}` : `<${el.tagName.toLowerCase()}>`;
                    inlineDodgy.push(`INLINE_PX: ${identifier}`);

                    // SNITCH: Apply the visual highlight
                    el.classList.add('design-helper-inline-liar');
                }

                if (/:\s*[1-9]\d*\.?\d*px|:\s*0\.\d*[1-9]em/.test(styleAttr)) {
                    const identifier = el.id ? `#${el.id}` : `<${el.tagName.toLowerCase()}>`;
                    inlineDodgy.push(`INLINE_EM: ${identifier}`);

                    // SNITCH: Apply the visual highlight
                    el.classList.add('design-helper-inline-liar');
                }
                if (/:\s*\d+\.(?!(0|25|5|75)0*\s*pt)\d+pt/.test(styleAttr)) {
                    const identifier = el.id ? `#${el.id}` : `<${el.tagName.toLowerCase()}>`;
                    inlineDodgy.push(`DIRTY_PRECISION: ${identifier}`);

                    // SNITCH: Apply the visual highlight
                    el.classList.add('design-helper-inline-liar');
                }

            });
            return [...new Set(inlineDodgy)];
        };



        // Then inside the observer's requestAnimationFrame:
        const pxIssues = scanForLiarUnits();
        pxIssues.forEach(issue => allIssues.add(issue));

        // 2. Audit Inline Styles (The new bit)
        const inlineIssues = scanForInlineLiarUnits();
        inlineIssues.forEach(issue => allIssues.add(issue));


        updateStatusPanel(Array.from(allIssues));
    });
});



const createDesignStatusPanel = () => {
    const panel = document.createElement('div');
    panel.id = 'design-status-panel';
    panel.className = 'design-helper';
    panel.style.cssText = `
    position: fixed;
    bottom: 7.5pt;
    right: 7.5pt;
    background: #f0f0f0;
    border: 1pt solid #ccc;
    padding: 7.5pt;
    border-radius: 2.5pt;
    font-family: monospace;
    font-size: 10pt;
    max-width: 225pt;
    max-height: 150pt;
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
    ctx.font = `16pt "${fontName}", "monospace"`;
    ctx.fillText(testString, 10, 50);
    const withFont = ctx.getImageData(0, 0, canvas.width, canvas.height).data;

    // Draw with fallback only
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    ctx.font = `16pt "monospace"`;

    ctx.fillText(testString, 10, 50);
    const withFallback = ctx.getImageData(0, 0, canvas.width, canvas.height).data;

    return !pixelsEqual(withFont, withFallback);
}

function pixelsEqual(a, b) {
    for (let i = 0; i < a.length; i++)
        if (a[i] !== b[i]) return false;
    return true;
}
