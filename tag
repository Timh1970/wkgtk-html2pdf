#!/usr/bin/env bash
set -euo pipefail

conf_file="version.conf"

# Default suffix
suffix=""

# Parse command-line arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    -t|--type)
      shift
      case ${1:-} in
        dev|rc1|rel)
          suffix="$1"
          ;;
        *)
          echo "Error: Suffix must be 'dev', 'rc1', or 'rel'. Got '${1:-}'." >&2
          exit 1
          ;;
      esac
      shift
      ;;
    *)
      echo "Unknown option: $1" >&2
      exit 1
      ;;
  esac
done

if [ -z "$suffix" ]; then
  echo "Error: --type must be provided (dev, rc1, or rel)" >&2
  exit 1
fi

# No suffix for release
if [[ $suffix == "rel" ]]; then
  suffix=""
fi

# Ensure we run from the repository root
repo_root=$(git rev-parse --show-toplevel)
cd "$repo_root" || exit 1

# Ensure version.conf exists and has defaults
if [[ ! -f "$conf_file" ]]; then
  cat > "$conf_file" <<EOF
major=0
minor=0
patch=0
EOF
fi

# Source version file (safe)
# shellcheck disable=SC1090
source "$conf_file"

# Ensure numeric values (fallback to 0)
major=${major:-0}
minor=${minor:-0}
patch=${patch:-0}

# Increment patch
patch=$((patch + 1))

# Save updated version
cat > "$conf_file" <<EOF
major=$major
minor=$minor
patch=$patch
EOF

# Build version string
name="wk2gtkpdf"
version="$major.$minor.$patch"

# Replace @VERSION@ in templates
sed "s/@VERSION@/${version}/" ${name}.pc.in > ./src/${name}/${name}.pc


# 1. Prepare the Debian-style date (Required for the footer)
# Format: Mon, 22 Feb 2026 22:45:00 +0000
DEB_DATE=$(date -R)

# 2. Build the new Header and Footer
# Note: 'unstable' or 'trixie' is the target distribution
CHANGELOG_HEADER="wkgtk-html2pdf (${version}-1) trixie; urgency=medium"
CHANGELOG_FOOTER=" -- James Hothersall <james@wkgtk-html2pdf.com>  $DEB_DATE"

# 3. Prepend to the existing changelog
# We create a temporary file, add the new entry, then append the old content
if [ -f "debian/changelog" ]; then
  echo "$CHANGELOG_HEADER" > debian/changelog.new
  echo "" >> debian/changelog.new
  echo "  * Automated release: Version ${version}" >> debian/changelog.new
  echo "" >> debian/changelog.new
  echo "$CHANGELOG_FOOTER" >> debian/changelog.new
  echo "" >> debian/changelog.new
  cat debian/changelog >> debian/changelog.new
  mv debian/changelog.new debian/changelog
fi


# This replaces the brittle .symbols file with a stable versioned dependency.
SHLIBS_FILE="debian/libwk2gtkpdf0.shlibs"
echo "libwk2gtkpdf 0 libwk2gtkpdf0 (>= ${version})" > "$SHLIBS_FILE"

# Sanitise the debian files
sed -i 's/[[:space:]]*$//' debian/control debian/changelog debian/rules

# Stage everything (intentional)
git add .

# Commit if any staged changes exist
if git diff --cached --quiet; then
  echo "No changes to commit (files unchanged)."
else
  git commit -m "Bump version to ${version}"
fi

# Build tag name
tag="v${version}"
[[ -n "$suffix" ]] && tag="${tag}-$suffix"

# Prevent accidental duplicate tag (remove -f to be strict)
if git rev-parse -q --verify "refs/tags/${tag}" >/dev/null; then
  echo "Tag ${tag} already exists. Use -f to force or remove the existing tag first." >&2
  exit 1
fi

# Create annotated tag
git tag -a "$tag" -m "Release $tag"


# Define the names exactly as the Debian Site Inspector expects
DEB_NAME="wkgtk-html2pdf"
DEB_ARCHIVE_NAME="${DEB_NAME}_${version}.orig.tar.gz"

# Create the archive (with hyphen in prefix, underscore in filename)
git archive --format=tar.gz --prefix="${DEB_NAME}-${version}/" -o "../$DEB_ARCHIVE_NAME" "$tag"

echo "---------------------------------------------------"
echo "Version: $version"
echo "Tag created: $tag"
echo "Debian-ready Archive: $DEB_ARCHIVE_NAME (created in parent folder)"
echo "---------------------------------------------------"
