#!/bin/bash
set -e
set -o pipefail

if [[ -z "$OUT_DIR" ]]; then
    OUT_DIR="cache"
fi

if [[ -n "$REPOSITORY" ]]; then
    TARGET_REPOSITORY=$REPOSITORY
else
    if [[ -z "$GITHUB_REPOSITORY" ]]; then
        echo "Set the GITHUB_REPOSITORY env variable."
        exit 1
    fi
    TARGET_REPOSITORY=${GITHUB_REPOSITORY}
fi

if [[ -z "$GITHUB_HOSTNAME" ]]; then
    GITHUB_HOSTNAME="github.com"
fi

if [! -d "./cache" ]; then
    mkdir cache
if

echo "Starting deploy..."

git config --global url."https://".insteadOf git://
## $GITHUB_SERVER_URL is set as a default environment variable in all workflows, default is https://github.com
git config --global url."$GITHUB_SERVER_URL/".insteadOf "git@${GITHUB_HOSTNAME}":

# needed or else we get 'doubious ...' error
echo "Disable safe directory check"
git config --global --add safe.directory '*'

wget -O cache/ithome-rss.xml "https://www.ithome.com/rss"
wget -O cache/the-paper.json "https://cache.thepaper.cn/contentapi/wwwIndex/rightSidebar"

echo "Pushing artifacts to ${TARGET_REPOSITORY}:main"
git config user.name "GitHub Actions"
git config user.email "github-actions-bot@users.noreply.${GITHUB_HOSTNAME}"
git add .
git commit -m "update news files."
git push --force origin
echo "update complete"
