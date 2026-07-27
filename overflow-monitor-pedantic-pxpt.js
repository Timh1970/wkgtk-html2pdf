let canvas, ctx;
const allIssues = new Set();
const realFonts = [];


const sheet = new CSSStyleSheet();
sheet.replaceSync(`
    .design-helper-inline-liar {
        outline: 2pt dashed #ae3ec9 !important;
        outline-offset: -2pt;
        position: relative;
    }

    .design-helper-inline-liar::after {
        content: attr(data-label);
        position: absolute;
        top: 0;
        right: 0;
        background: #ae3ec9;
        color: white;
        font-size: 7pt;
        padding: 0.75pt 3pt;
        z-index: 9999;
        pointer-events: none;
    }
`);

document.adoptedStyleSheets = [sheet];

const injectLinterStyles = () => {
    const styleId = 'linter-diagnostic-styles';
    if (document.getElementById(styleId)) return;

    const style = document.createElement('style');
    style.id = styleId;
    style.textContent = `
        /* Diagnostics: Locks the red overflow box strictly to the visible page boundaries */
        .subpage.has-overflow {
            position: relative !important;
        }
        .subpage.has-overflow::after {
            content: "" !important;
            position: absolute !important;
            top: 0 !important;
            left: 0 !important;
            width: 100% !important;
            height: 100% !important;
            box-sizing: border-box !important;
            pointer-events: none !important;
            z-index: 9999 !important;
            border: 8.25pt dashed red !important;
        }
    `;
    document.head.appendChild(style);
};

injectLinterStyles();

async function initDesignHelper() {
    console.log("🛠 Starting Design Helper...");
    await document.fonts.ready;
    await applyTextShave();
    await new Promise(resolve => requestAnimationFrame(resolve));
    document.querySelectorAll('.subpage').forEach((subpage) => {
        observer.observe(subpage);
    });
    console.log("✅ Observer active on corrected metrics.");
};



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
    initViewportPersistence();

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
        //element.style.border = "3pt solid red";
        // element.style.outline = "8pt solid red";
        // element.style.outlineOffset = "0.75pt"; // Pulls it inside so it doesn't bleed off-page
        element.classList.add('has-overflow');
    } else {
        element.classList.remove('has-overflow');
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

        // 3. Audit Tables
        const tableIssues = scanTables();
        tableIssues.forEach(issue => allIssues.add(issue));

        // Then inside the observer's requestAnimationFrame:
        const pxIssues = scanForLiarUnits();
        pxIssues.forEach(issue => allIssues.add(issue));

        // 2. Audit Inline Styles (The new bit)
        const inlineIssues = scanForInlineLiarUnits();
        inlineIssues.forEach(issue => allIssues.add(issue));


        updateStatusPanel(Array.from(allIssues));
    });
});

const initViewportPersistence = () => {
    // 1. Restore exact scroll position on layout load
    const savedScrollTop = localStorage.getItem('precision_layout_scroll_pos');
    if (savedScrollTop) {
        window.scrollTo(0, parseFloat(savedScrollTop));
    }

    // 2. Track scroll changes and save them instantly
    window.addEventListener('scroll', () => {
        localStorage.setItem('precision_layout_scroll_pos', window.scrollY);
    });
};

const scanForLiarUnits = () => {
    const dodgySelectors = [];
    const cascadeRegistry = {}; // Tracks normalized state: { selector: { fontSizePx: X, lineHeightPx: Y } }

    // Unified helper: Normalizes any pt or px unit string cleanly down to standard Pixels
    const parseToPxValue = (styleBlock, property) => {
        const val = styleBlock[property];
        if (!val) return null;

        const cleanVal = val.trim().toLowerCase();
        if (cleanVal.includes('px')) {
            return parseFloat(cleanVal);
        }
        if (cleanVal.includes('pt')) {
            // pt to px is a crisp 1.3333... scalar mapping
            return parseFloat(cleanVal) * (4 / 3);
        }
        return null;
    };

    // Helper to evaluate layout parity for a resolved pair using true px targets
    const verifyParityFromPx = (fontSizePx, lineHeightPx, contextLabel) => {
        // Enforce integer crispness for Skia raster boundaries
        const isFontSizePxInt = Math.abs(fontSizePx - Math.round(fontSizePx)) < 0.0001;
        const isLineHeightPxInt = Math.abs(lineHeightPx - Math.round(lineHeightPx)) < 0.0001;

        if (!isFontSizePxInt || !isLineHeightPxInt) {
            // Convert back to pt format ONLY for the error report message to match context
            const fSizePt = (fontSizePx * 0.75).toFixed(2);
            const lHeightPt = (lineHeightPx * 0.75).toFixed(2);
            dodgySelectors.push(
                `SUBPIXEL_FONT_METRIC (font-size: ${fSizePt}pt [${fontSizePx.toFixed(1)}px], line-height: ${lHeightPt}pt [${lineHeightPx.toFixed(1)}px] forces fractional layout grid): ${contextLabel}`
            );
            return;
        }

        const totalLeadingPx = Math.round(lineHeightPx) - Math.round(fontSizePx);
        if (totalLeadingPx % 2 !== 0) {
            const topHalfLeading = totalLeadingPx / 2;
            const fSizePt = (fontSizePx * 0.75).toFixed(2);
            const lHeightPt = (lineHeightPx * 0.75).toFixed(2);
            dodgySelectors.push(
                `ODD_LEADING_TRAP (font: ${fSizePt}pt [${Math.round(fontSizePx)}px], lh: ${lHeightPt}pt [${Math.round(lineHeightPx)}px] -> Leading is ${totalLeadingPx}px, splitting into unsafe ${topHalfLeading}px half-leading): ${contextLabel}`
            );
        }
    };

    // Recursive stylesheet scanner rule-processor
    const processRule = (rule) => {
        try {
            if (rule.media) {
                const mediaType = rule.media.mediaText.toLowerCase();
                if (mediaType.includes('screen') && !mediaType.includes('print') && !mediaType.includes('all')) {
                    dodgySelectors.push(`SCREEN_ONLY_MEDIA_QUERY: @media ${rule.media.mediaText}`);
                }
                if (rule.cssRules) {
                    Array.from(rule.cssRules).forEach(nestedRule => processRule(nestedRule));
                }
                return;
            }

            if (!rule.style || !rule.selectorText) return;
            if (rule.selectorText.includes('design-helper')) return;

            const text = rule.cssText ? rule.cssText.toLowerCase() : '';
            if (!text) return;

            // 3. IDENTIFY FRACTIONAL PX UNITS ONLY (Safe integer px bypasses check)
            if (/\b\d+\.\d+px\b/i.test(text)) {
                dodgySelectors.push(`FRACTIONAL_PX_UNIT: ${rule.selectorText}`);
            }

            // 4. BAN EM & REM UNITS
            if (/\b\d*\.?\d+rem\b/.test(text)) {
                dodgySelectors.push(`REM_UNIT: ${rule.selectorText}`);
            } else if (/\b\d*\.?\d+em\b/.test(text)) {
                dodgySelectors.push(`EM_UNIT: ${rule.selectorText}`);
            }

            // 5. COMPREHENSIVE COMPONENT PRECISION CHECK
            const propertyBlockRegex = /([\w-]+)\s*:\s*([^;}\n]+)/g;
            let propMatch;

            while ((propMatch = propertyBlockRegex.exec(text)) !== null) {
                const propName = propMatch[1];
                const rawValueBlock = propMatch[2];

                const ptMatches = rawValueBlock.match(/\b\d*\.?\d+pt\b/g);
                const pxMatches = rawValueBlock.match(/\b\d*\.?\d+px\b/g);

                if (ptMatches) {
                    ptMatches.forEach(ptString => {
                        const val = parseFloat(ptString);
                        const convertedPx = val * (4 / 3);
                        const roundedPx = Number(convertedPx.toFixed(4));
                        if (!Number.isInteger(roundedPx)) {
                            dodgySelectors.push(`DIRTY_PRECISION (${propName} has unsafe token '${ptString}' -> subpixel ${convertedPx.toFixed(2)}px): ${rule.selectorText}`);
                        }
                    });
                }

                if (pxMatches) {
                    pxMatches.forEach(pxString => {
                        const val = parseFloat(pxString);
                        if (!Number.isInteger(val)) {
                            dodgySelectors.push(`DIRTY_PX_PRECISION (${propName} has fractional pixel '${pxString}'): ${rule.selectorText}`);
                        }
                    });
                }
            }

            // 6. NATIVE STYLE AUDIT
            const rawLineHeight = rule.style.lineHeight;
            if (rawLineHeight && rawLineHeight.trim() !== '') {
                const trimmedValue = rawLineHeight.trim();
                if (trimmedValue.includes('%')) {
                    dodgySelectors.push(`PERCENTAGE_UNIT (line-height: ${trimmedValue}): ${rule.selectorText}`);
                } else if (!isNaN(trimmedValue) && parseFloat(trimmedValue) !== 0) {
                    dodgySelectors.push(`UNITLESS_VALUE (line-height: ${trimmedValue}): ${rule.selectorText}`);
                }
            }

            // 7. CASCADE REGISTRY EXTRACTION (Now cleanly unified via Px normalizing)
            const fSizePx = parseToPxValue(rule.style, 'fontSize');
            const lHeightPx = parseToPxValue(rule.style, 'lineHeight');

            const selectors = rule.selectorText.split(',');
            selectors.forEach(sel => {
                const cleanSel = sel.trim();
                if (!cascadeRegistry[cleanSel]) cascadeRegistry[cleanSel] = {};
                if (fSizePx !== null) cascadeRegistry[cleanSel].fontSizePx = fSizePx;
                if (lHeightPx !== null) cascadeRegistry[cleanSel].lineHeightPx = lHeightPx;
            });

            if (fSizePx !== null && lHeightPx !== null) {
                verifyParityFromPx(fSizePx, lHeightPx, rule.selectorText);
            }

        } catch (ruleException) {
            console.debug("Skipped non-standard style rule token alignment check:", ruleException);
        }
    };

    // Main stylesheet iterator loop
    Array.from(document.styleSheets).forEach(sheet => {
        try {
            if (sheet.href) {
                const hrefLower = sheet.href.toLowerCase();
                if (hrefLower.startsWith('chrome') || hrefLower.startsWith('resource')) return;
                const isLocalDev = window.location.protocol === 'file:';
                const isSameDomain = window.location.hostname && hrefLower.includes(window.location.hostname.toLowerCase());
                if (!isLocalDev && !isSameDomain) return;
            }
            if (sheet.cssRules) {
                Array.from(sheet.cssRules).forEach(rule => processRule(rule));
            }
        } catch (stylesheetException) {
            console.debug("Linter styleSheet context evaluation skipped:", stylesheetException);
        }
    });

    // Pass 2: Evaluate cascade inheritance using clean pixel metrics
    Object.keys(cascadeRegistry).forEach(selector => {
        const metrics = cascadeRegistry[selector];

        if (metrics.lineHeightPx && !metrics.fontSizePx) {
            let inheritedFontSizePx = null;
            const potentialParents = ['body', 'html', '.page', '.container', 'main'];

            // Match structural parent properties inside the local file registry
            for (const parent of potentialParents) {
                if (cascadeRegistry[parent] && cascadeRegistry[parent].fontSizePx) {
                    inheritedFontSizePx = cascadeRegistry[parent].fontSizePx;
                    break;
                }
            }

            if (inheritedFontSizePx !== null) {
                verifyParityFromPx(inheritedFontSizePx, metrics.lineHeightPx, `${selector} (inherits font-size)`);
            }
        }
    });

    return dodgySelectors;
};

//NEWER PX AND PT VERSION
// const scanForLiarUnits = () => {
//     const dodgySelectors = [];
//     const cascadeRegistry = {}; // Tracks: { selector: { fontSize: X, lineHeight: Y } }
//
//     // Helper to safely extract clean pt values from a rule block
//     const getPtValue = (styleBlock, property) => {
//         const val = styleBlock[property];
//         if (val && val.trim().toLowerCase().includes('pt')) {
//             return parseFloat(val);
//         }
//         return null;
//     };
//
//     // Helper to evaluate layout parity for a resolved pair
//     const verifyParity = (fontSizePt, lineHeightPt, contextLabel) => {
//         const fontSizePx = fontSizePt / 0.75;
//         const lineHeightPx = lineHeightPt / 0.75;
//
//         const isFontSizePxInt = Math.abs(fontSizePx - Math.round(fontSizePx)) < 0.0001;
//         const isLineHeightPxInt = Math.abs(lineHeightPx - Math.round(lineHeightPx)) < 0.0001;
//
//         if (!isFontSizePxInt || !isLineHeightPxInt) {
//             dodgySelectors.push(
//                 `SUBPIXEL_FONT_METRIC (font-size: ${fontSizePt}pt, line-height: ${lineHeightPt}pt forces fractional layout grid): ${contextLabel}`
//             );
//             return;
//         }
//
//         const totalLeadingPx = Math.round(lineHeightPx) - Math.round(fontSizePx);
//         if (totalLeadingPx % 2 !== 0) {
//             const topHalfLeading = totalLeadingPx / 2;
//             dodgySelectors.push(
//                 `ODD_LEADING_TRAP (font: ${fontSizePt}pt [${Math.round(fontSizePx)}px], lh: ${lineHeightPt}pt [${Math.round(lineHeightPx)}px] -> Leading is ${totalLeadingPx}px, splitting into unsafe ${topHalfLeading}px half-leading): ${contextLabel}`
//             );
//         }
//     };
//
//     // Recursive stylesheet scanner rule-processor
//     const processRule = (rule) => {
//         try {
//             // 1. DANGEROUS MEDIA QUERY TRAP & RECURSION BRANCH
//             if (rule.media) {
//                 const mediaType = rule.media.mediaText.toLowerCase();
//                 if (mediaType.includes('screen') && !mediaType.includes('print') && !mediaType.includes('all')) {
//                     dodgySelectors.push(`SCREEN_ONLY_MEDIA_QUERY: @media ${rule.media.mediaText}`);
//                 }
//
//                 if (rule.cssRules) {
//                     Array.from(rule.cssRules).forEach(nestedRule => processRule(nestedRule));
//                 }
//                 return;
//             }
//
//             // Standard validation guard for styling rules
//             if (!rule.style || !rule.selectorText) return;
//
//             // 2. IGNORE SYSTEM UI: Keep the linter panel from reporting its own styling
//             if (rule.selectorText.includes('design-helper')) {
//                 return;
//             }
//
//             const text = rule.cssText ? rule.cssText.toLowerCase() : '';
// if (!text) return;
//
// // 3. IDENTIFY FRACTIONAL PX UNITS ONLY (Safe integer px bypasses check)
// // Matches any pixel token containing a dot followed by numbers (e.g., 12.5px, 0.25px)
// if (/\b\d+\.\d+px\b/i.test(text)) {
//     dodgySelectors.push(`FRACTIONAL_PX_UNIT: ${rule.selectorText}`);
// }
//
// // 4. BAN EM & REM UNITS
// if (/\b\d*\.?\d+rem\b/.test(text)) {
//     dodgySelectors.push(`REM_UNIT: ${rule.selectorText}`);
// } else if (/\b\d*\.?\d+em\b/.test(text)) {
//     dodgySelectors.push(`EM_UNIT: ${rule.selectorText}`);
// }
//
// // 5. COMPREHENSIVE COMPONENT PRECISION CHECK (Evaluates both pt and px values)
// const propertyBlockRegex = /([\w-]+)\s*:\s*([^;}\n]+)/g;
// let propMatch;
//
// while ((propMatch = propertyBlockRegex.exec(text)) !== null) {
//     const propName = propMatch[1];
//     const rawValueBlock = propMatch[2];
//
//     // Track down any point values or pixel tokens in the declaration block
//     const ptMatches = rawValueBlock.match(/\b\d*\.?\d+pt\b/g);
//     const pxMatches = rawValueBlock.match(/\b\d*\.?\d+px\b/g);
//
//     // A. Legacy Point Checker
//     if (ptMatches) {
//         ptMatches.forEach(ptString => {
//             const val = parseFloat(ptString);
//             const convertedPx = val * (4 / 3);
//             const roundedPx = Number(convertedPx.toFixed(4));
//
//             if (!Number.isInteger(roundedPx)) {
//                 dodgySelectors.push(
//                     `DIRTY_PRECISION (${propName} has unsafe token '${ptString}' -> subpixel ${convertedPx.toFixed(2)}px): ${rule.selectorText}`
//                 );
//             }
//         });
//     }
//
//     // B. Modern Pixel Checker
//     if (pxMatches) {
//         pxMatches.forEach(pxString => {
//             const val = parseFloat(pxString);
//             // Catch floating-point or explicit decimals in layout math (e.g., 14.5px)
//             if (!Number.isInteger(val)) {
//                 dodgySelectors.push(
//                     `DIRTY_PX_PRECISION (${propName} has fractional pixel '${pxString}'): ${rule.selectorText}`
//                 );
//             }
//         });
//     }
// }
//
//
//             // 6. NATIVE STYLE AUDIT (Unit checking for invalid fluid/relative lines)
//             const rawLineHeight = rule.style.lineHeight;
//             if (rawLineHeight && rawLineHeight.trim() !== '') {
//                 const trimmedValue = rawLineHeight.trim();
//
//                 if (trimmedValue.includes('%')) {
//                     dodgySelectors.push(`PERCENTAGE_UNIT (line-height: ${trimmedValue}): ${rule.selectorText}`);
//                 } else if (!isNaN(trimmedValue) && parseFloat(trimmedValue) !== 0) {
//                     dodgySelectors.push(`UNITLESS_VALUE (line-height: ${trimmedValue}): ${rule.selectorText}`);
//                 }
//             }
//
//             // 7. CASCADE REGISTRY EXTRACTION
//             const fSize = getPtValue(rule.style, 'fontSize');
//             const lHeight = getPtValue(rule.style, 'lineHeight');
//
//             // Split comma-separated rules to track selectors cleanly
//             const selectors = rule.selectorText.split(',');
//             selectors.forEach(sel => {
//                 const cleanSel = sel.trim();
//                 if (!cascadeRegistry[cleanSel]) cascadeRegistry[cleanSel] = {};
//                 if (fSize !== null) cascadeRegistry[cleanSel].fontSize = fSize;
//                 if (lHeight !== null) cascadeRegistry[cleanSel].lineHeight = lHeight;
//             });
//
//             // Local context verification if both properties are explicitly paired on this specific rule
//             if (fSize !== null && lHeight !== null) {
//                 verifyParity(fSize, lHeight, rule.selectorText);
//             }
//
//         } catch (ruleException) {
//             console.debug("Skipped non-standard style rule token alignment check:", ruleException);
//         }
//     };
//
//     // Main stylesheet iterator loop (Pass 1: Collects tokens & builds the flat registry)
//     Array.from(document.styleSheets).forEach(sheet => {
//         try {
//             if (sheet.href) {
//                 const hrefLower = sheet.href.toLowerCase();
//
//                 if (hrefLower.startsWith('chrome') || hrefLower.startsWith('resource')) {
//                     return;
//                 }
//
//                 const isLocalDev = window.location.protocol === 'file:';
//                 const isSameDomain = window.location.hostname && hrefLower.includes(window.location.hostname.toLowerCase());
//
//                 if (!isLocalDev && !isSameDomain) {
//                     return;
//                 }
//             }
//
//             if (sheet.cssRules) {
//                 Array.from(sheet.cssRules).forEach(rule => processRule(rule));
//             }
//         } catch (stylesheetException) {
//             console.debug("Linter styleSheet context evaluation skipped:", stylesheetException);
//         }
//     });
//
//     // Pass 2: Statically cross-examine elements missing a local font-size block against the cascade
//     Object.keys(cascadeRegistry).forEach(selector => {
//         const metrics = cascadeRegistry[selector];
//
//         // Evaluate rule blocks that declare a line-height but depend on structural inheritance for font sizing
//         if (metrics.lineHeight && !metrics.fontSize) {
//             let inheritedFontSize = null;
//
//             // Search hierarchy: Check root scopes, structural containers, and explicit parent classes in complex selectors
//             const potentialParents = ['body', 'html', '.page', '.container', 'main'];
//
//             // If the selector points to a nested node (e.g. ".parent .child" or ".parent > .child")
//             // dynamically extract the parent chain string to search the registry
//             const parentChainMatch = selector.match(/(.+)\s+[>\s*]?\s+[\w-.:#]+/);
//             if (parentChainMatch && parentChainMatch[1]) {
//                 potentialParents.unshift(parentChainMatch[1].trim());
//             }
//
//             // Extract the closest available parent context matching our registered rules
//             for (const parent of potentialParents) {
//                 if (cascadeRegistry[parent] && cascadeRegistry[parent].fontSize) {
//                     inheritedFontSize = cascadeRegistry[parent].fontSize;
//                     break;
//                 }
//             }
//
//             if (inheritedFontSize !== null) {
//                 verifyParity(inheritedFontSize, metrics.lineHeight, `${selector} (inheriting font-size: ${inheritedFontSize}pt from layout context)`);
//             } else {
//                 dodgySelectors.push(
//                     `UNRESOLVED_FONT_CONTEXT (line-height defined as ${metrics.lineHeight}pt but no valid structural parent font-size found in sheet): ${selector}`
//                 );
//             }
//         }
//     });
//
//     return [...new Set(dodgySelectors)];
// };

// ORIGINAL PT ONLY VERSION
// const scanForLiarUnits = () => {
//     const dodgySelectors = [];
//
//     // Helper function to process rules recursively
//     const processRule = (rule) => {
//         try {
//             // 1. DANGEROUS MEDIA QUERY TRAP & RECURSION BRANCH
//             if (rule.media) {
//                 const mediaType = rule.media.mediaText.toLowerCase();
//                 if (mediaType.includes('screen') && !mediaType.includes('print') && !mediaType.includes('all')) {
//                     dodgySelectors.push(`SCREEN_ONLY_MEDIA_QUERY: @media ${rule.media.mediaText}`);
//                 }
//
//                 // Unpack and process the rules nested inside the media block safely
//                 if (rule.cssRules) {
//                     Array.from(rule.cssRules).forEach(nestedRule => processRule(nestedRule));
//                 }
//                 return;
//             }
//
//             // Standard validation guard for styling rules
//             // Wrapped carefully inside the try block to avoid strict type crashes
//             if (!rule.style || !rule.selectorText) return;
//
//             // 2. IGNORE SYSTEM UI: Keep the linter panel from reporting its own styling
//             if (rule.selectorText.includes('design-helper')) {
//                 return;
//             }
//
//             const text = rule.cssText ? rule.cssText.toLowerCase() : '';
//             if (!text) return;
//
//             // 3. BAN PX UNITS
//             if (/:\s*[1-9]\d*\.?\d*px|:\s*0\.\d*[1-9]\s*px/.test(text)) {
//                 dodgySelectors.push(`PX_UNIT: ${rule.selectorText}`);
//             }
//
//             // 4. BAN EM & REM UNITS
//             if (/\b\d*\.?\d+rem\b/.test(text)) {
//                 dodgySelectors.push(`REM_UNIT: ${rule.selectorText}`);
//             } else if (/\b\d*\.?\d+em\b/.test(text)) {
//                 dodgySelectors.push(`EM_UNIT: ${rule.selectorText}`);
//             }
//
//             // 5. POINT PRECISION CHECK
//             const propertyBlockRegex = /([\w-]+)\s*:\s*([^;}\n]+)/g;
//             let propMatch;
//
//             while ((propMatch = propertyBlockRegex.exec(text)) !== null) {
//                 const propName = propMatch[1];
//                 const rawValueBlock = propMatch[2];
//
//                 // Find every individual point string inside this specific property's values
//                 const ptMatches = rawValueBlock.match(/\b\d*\.?\d+pt\b/g);
//
//                 if (ptMatches) {
//                     ptMatches.forEach(ptString => {
//                         const val = parseFloat(ptString);
//                         const convertedPx = val * (4 / 3);
//                         const roundedPx = Number(convertedPx.toFixed(4));
//
//                         if (!Number.isInteger(roundedPx)) {
//                             dodgySelectors.push(
//                                 `DIRTY_PRECISION (${propName} has unsafe token '${ptString}' -> subpixel ${convertedPx.toFixed(2)}px): ${rule.selectorText}`
//                             );
//                         }
//                     });
//                 }
//             }
//
//             // 6. NATIVE STYLE AUDIT (line-height, etc.)
//             const targetedProps = ['lineHeight'];
//
//             targetedProps.forEach(prop => {
//                 const rawValue = rule.style[prop];
//                 if (rawValue && rawValue !== '') {
//                     const trimmedValue = rawValue.trim();
//
//                     if (trimmedValue.includes('%')) {
//                         dodgySelectors.push(`PERCENTAGE_UNIT (${prop}: ${trimmedValue}): ${rule.selectorText}`);
//                     } else if (!isNaN(trimmedValue) && parseFloat(trimmedValue) !== 0) {
//                         dodgySelectors.push(`UNITLESS_VALUE (${prop}: ${trimmedValue}): ${rule.selectorText}`);
//                     }
//                 }
//             });
//         } catch (ruleException) {
//             // Catches strict-mode properties anomalies from specific rule types
//             // and lets the loop smoothly continue auditing everything else
//             console.debug("Skipped non-standard style rule token alignment check:", ruleException);
//         }
//     };
//
//
//     // Main stylesheet iterator loop
//     Array.from(document.styleSheets).forEach(sheet => {
//         try {
//             // 1. RESOLVE SECURE HREF PASS
//             // If running remotely on a local file, sheet.href might be null,
//             // but the sheet is still our local template stylesheet.
//             if (sheet.href) {
//                 const hrefLower = sheet.href.toLowerCase();
//
//                 // Absolute skip for browser internals or external non-app metrics
//                 if (hrefLower.startsWith('chrome') || hrefLower.startsWith('resource')) {
//                     return;
//                 }
//
//                 // If we are developing locally, allow any local stylesheets to pass
//                 const isLocalDev = window.location.protocol === 'file:';
//                 const isSameDomain = window.location.hostname && hrefLower.includes(window.location.hostname.toLowerCase());
//
//                 if (!isLocalDev && !isSameDomain) {
//                     return; // Skip genuine third-party external CDNs in production
//                 }
//             }
//
//             // 2. SAFETY GAUNTLET PASS
//             const status = checkStylesheetAccess(sheet);
//             const sheetName = sheet.href ? sheet.href.split('/').pop() : 'inline-style';
//
//             if (!status.accessible) {
//                 allIssues.add(`READ_ERROR: [${sheetName}] - ${status.reason}`);
//                 console.warn(`Linter cannot audit: ${sheetName}. ${status.reason}`);
//                 return; // Skip this sheet                return;
//             }
//
//             // 3. EXECUTE DEEP SCAN
//             if (sheet.cssRules) {
//                 Array.from(sheet.cssRules).forEach(rule => processRule(rule));
//             }
//         } catch (stylesheetException) {
//             // Catches strict cross-origin security context blocks silently
//             console.debug("Linter styleSheet context evaluation skipped:", stylesheetException);
//         }
//     });
//     return [...new Set(dodgySelectors)];
// };



// const scanForInlineLiarUnits = () => {
//     const inlineDodgy = [];
//
//     // First, clear any old snitch classes so we start fresh
//     document.querySelectorAll('.design-helper-inline-liar')
//         .forEach(el => el.classList.remove('design-helper-inline-liar'));
//
//     const elementsWithStyle = document.querySelectorAll('[style]');
//
//     elementsWithStyle.forEach(el => {
//         const styleAttr = el.getAttribute('style').toLowerCase();
//
//         // Use your "Non-Zero" regex
//         if (/:\s*[1-9]\d*\.?\d*px|:\s*0\.\d*[1-9]px/.test(styleAttr)) {
//             const identifier = el.id ? `#${el.id}` : `<${el.tagName.toLowerCase()}>`;
//             inlineDodgy.push(`INLINE_PX: ${identifier}`);
//
//             // SNITCH: Apply the visual highlight
//             el.setAttribute('data-label', 'INLINE_PX');
//             el.classList.add('design-helper-inline-liar');
//         }
//
//         if (/:\s*[1-9]\d*\.?\d*em|:\s*0\.\d*[1-9]em/.test(styleAttr)) {
//             const identifier = el.id ? `#${el.id}` : `<${el.tagName.toLowerCase()}>`;
//             inlineDodgy.push(`INLINE_EM: ${identifier}`);
//
//             // SNITCH: Apply the visual highlight
//             el.setAttribute('data-label', 'INLINE_EM');
//
//             el.classList.add('design-helper-inline-liar');
//         }
//
//         if (!allIssues.has('OVERFLOW')) {
//
//             const ptMatches = styleAttr.match(/(\d*\.?\d+)pt/g);
//             if (ptMatches) {
//                 ptMatches.forEach(match => {
//                     const val = match.replace('pt', '');
//                     if (isDirtyPT(val)) {
//                         inlineDodgy.push(`DIRTY_PRECISION (${val}pt) inline`);
//                         el.setAttribute('data-label', `DIRTY_PRECISION: ${val}pt`);
//                         el.classList.add('design-helper-inline-liar');
//                     }
//                 });
//             }
//         }
//
//     });
//
//     return [...new Set(inlineDodgy)];
// };

const scanForInlineLiarUnits = () => {
    const inlineDodgy = [];

    // First, clear any old snitch classes so we start fresh
    document.querySelectorAll('.design-helper-inline-liar')
        .forEach(el => el.classList.remove('design-helper-inline-liar'));

    const elementsWithStyle = document.querySelectorAll('[style]');

    elementsWithStyle.forEach(el => {
        const styleAttr = el.getAttribute('style').toLowerCase();
        const identifier = el.id ? `#${el.id}` : `<${el.tagName.toLowerCase()}>`;

        // 1. IDENTIFY FRACTIONAL INLINE PX UNITS ONLY (Safe integer px bypasses check)
        // Catches explicit dots with trailing fractional values like: style="margin: 4.5px;"
        if (/:\s*\d+\.\d+px/i.test(styleAttr)) {
            inlineDodgy.push(`FRACTIONAL_INLINE_PX: ${identifier}`);

            // SNITCH: Apply the visual highlight
            el.setAttribute('data-label', 'FRACTIONAL_PX');
            el.classList.add('design-helper-inline-liar');
        }

        // 2. BAN INLINE EM & REM UNITS (Unchanged logic)
        if (/:\s*[1-9]\d*\.?\d*em|:\s*0\.\d*[1-9]em/i.test(styleAttr)) {
            inlineDodgy.push(`INLINE_EM: ${identifier}`);

            // SNITCH: Apply the visual highlight
            el.setAttribute('data-label', 'INLINE_EM');
            el.classList.add('design-helper-inline-liar');
        }

        // 3. POINT PRECISION CHECK (Unchanged logic for legacy documents)
        if (!allIssues.has('OVERFLOW')) {
            const ptMatches = styleAttr.match(/(\d*\.?\d+)pt/g);
            if (ptMatches) {
                ptMatches.forEach(match => {
                    const val = match.replace('pt', '');
                    if (isDirtyPT(val)) {
                        inlineDodgy.push(`DIRTY_PRECISION (${val}pt) inline: ${identifier}`);
                        el.setAttribute('data-label', `DIRTY_PRECISION: ${val}pt`);
                        el.classList.add('design-helper-inline-liar');
                    }
                });
            }
        }
    });

    return [...new Set(inlineDodgy)];
};


const scanTables = () => {
    const inlineDodgy = [];
    if (!allIssues.has('OVERFLOW')) {
        // Select all potential layout elements regardless of current attributes
        const elements = document.querySelectorAll('table, td, th, col, div');

        elements.forEach(el => {
            // Manually check for both 'width' and 'height'
            const attrs = el.attributes;
            for (let i = 0; i < attrs.length; i++) {
                const attrName = attrs[i].name.toLowerCase();
                if (attrName === 'width' || attrName === 'height') {
                    const val = attrs[i].value; // Don't lowerCase yet, we need the original unit

                    // If it's on a server, we MUST see 'pt'
                    if (!val.toLowerCase().endsWith('pt')) {
                        inlineDodgy.push(`ATTR: <${el.tagName}> ${attrName}="${val}" is not PT`);
                        el.classList.add('design-helper-inline-liar');
                    } else if (isDirtyPT(val.replace(/pt/i, ''))) {
                        inlineDodgy.push(`DIRTY_PT_ATTR: <${el.tagName}> ${val}`);
                        el.classList.add('design-helper-inline-liar');
                    }
                }
            }
        });
    }
    return [...new Set(inlineDodgy)];
};

const isDirtyPT = (value) => {
    const num = parseFloat(value);
    if (isNaN(num)) return false;
    // The "Stone" Test: (Value / 0.75) must be an Integer
    return !Number.isInteger(Math.round((num / 0.75) * 1000) / 1000);
};

const checkStylesheetAccess = (sheet) => {
    try {
        // Attempting to read 'cssRules' is the definitive test.
        // Browsers like Blink will throw a DOMException (SecurityError)
        // if the file origin is considered "opaque" (like local files).
        const rules = sheet.cssRules || sheet.rules;

        if (rules === null) {
            return {
                accessible: false,
                reason: "BROWSER_RESTRICTED (null rules)"
            };
        }

        return {
            accessible: true,
            reason: null
        };
    } catch (e) {
        // Handle common security/access errors
        if (e.name === 'SecurityError' || e.code === 18) {
            return {
                accessible: false,
                reason: "CORS_OR_FILE_PROTOCOL_BLOCK"
            };
        }
        return {
            accessible: false,
            reason: "UNAVAILABLE (" + e.message + ")"
        };
    }
};



const createDesignStatusPanel = () => {
    const panel = document.createElement('div');
    panel.id = 'design-status-panel';
    panel.className = 'design-helper';
    panel.style.cssText = `
    position: fixed;
    bottom: 7.5pt;
    right: 7.5pt;
    background: #f0f0f0;
    border: 0.75pt solid #ccc;
    padding: 7.5pt;
    border-radius: 2.25pt;
    font-family: monospace;
    font-size: 9.75pt;
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

    // Convert Set to Array once for the UI logic
    const issuesArray = Array.from(allIssues);

    if (issuesArray.length === 0) {
        panel.style.display = 'none';
        return;
    }

    panel.style.display = 'block';

    // 1. Determine the "Master" color for borders
    let masterColor = 'brown';
    if (issuesArray.some(i => i.includes('OVERFLOW'))) {
        masterColor = 'red';
    } else if (issuesArray.some(i => i.includes('READ_ERROR'))) {
        masterColor = '#007bff'; // Blue for restricted access
    }

    panel.style.borderColor = masterColor;

    // 2. Generate the list with specific colors per issue type
    panel.innerHTML = `
        <strong style="color: #333;">Design Status Audit:</strong><br>
        ${issuesArray.map(issue => {
            let color = 'brown';
            if (issue.includes('OVERFLOW')) color = 'red';
            if (issue.includes('READ_ERROR')) color = '#007bff';

            return `<div style="color: ${color};">• ${issue}</div>`;
        }).join('')}
    `;

    // 3. Sync the subpage outlines to the master state
    document.querySelectorAll('.subpage').forEach((subpage) => {
        subpage.style.outlineColor = masterColor;
    });

    // 4. Show alert
    if (issuesArray.some(i => i.includes('READ_ERROR'))) {
        showPrecisionAlert();
    }
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

// ALERT USER IF BROWSER CANNOT READ FILE
const showPrecisionAlert = () => {
    // 1. Check if the user has already seen/dismissed this today
    if (sessionStorage.getItem('pedantic-linter-silenced')) return;

    const alertBox = document.createElement('div');
    alertBox.id = 'pedantic-alert-modal';

    alertBox.style = `
        position: fixed; top: 10.5pt; left: 50%; transform: translateX(-50%);
        width: 380.25pt; background: #fff; border: 2.25pt solid #007bff;
        padding: 18pt; z-index: 99999; box-shadow: 0 7.5pt 30pt rgba(0,0,0,0.4);
        font-family: var(--default-font-family); color: #333;
    `;

    alertBox.innerHTML = `
        <h3 style="color: #007bff; margin-top: 0; line-height: 18pt;">⚠ Precision Audit Locked</h3>
        <p style="font-size: 10.5pt; line-height: 15.75pt;">
            Your browser has prevented the <b>Pedantic Linter</b> from auditing some CSS files.
        </p>
        <ul style="font-size: 9.75pt; line-height: 15pt; margin: 15pt 0;">
            <li>Try an alternative browser. Or; </li>
            <li>Apply the appropriate flags or setting. Or;</li>
            <li>Host your files on a Web Server. Or;</li>
            <li>Switch to <b>overflow-monitor.js</b> for basic checks only.</li>
        </ul>
        <div style="margin-top: 18pt; display: flex; gap: 9pt;">
            <button id="dismiss-linter-alert" style="padding: 6pt 12pt; cursor: pointer;">Acknowledge</button>
            <button id="silence-linter-alert" style="padding: 6pt 12pt; background: #eee; border: 0.75pt solid #ccc; cursor: pointer;">Don't show again this session</button>
        </div>
    `;

    document.body.appendChild(alertBox);

    // Event Listeners
    document.getElementById('dismiss-linter-alert').onclick = () => alertBox.remove();
    document.getElementById('silence-linter-alert').onclick = () => {
        sessionStorage.setItem('pedantic-linter-silenced', 'true');
        alertBox.remove();
    };
};


// --- REDUNDANT FOR NOW --- //

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
        console.log("Chrome detected")
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
