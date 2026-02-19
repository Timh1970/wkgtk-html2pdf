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
sed "s/@VERSION@/${version}/" "${name}.pc.in" > "./src/${name}/${name}.pc"

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

# Create archive from the tag to guarantee it matches the tag
archive_name="inplicare-libs-${version}.tar.gz"
git archive --format=tar.gz --prefix="${name}-${version}/" "$tag" -o "$archive_name"

echo "Version: $version"
echo "Tag created: $tag"
echo "Archive created: $archive_name"
