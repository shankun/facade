#!/bin/bash
set -e
set -o pipefail

if [[ -z "$PAGES_BRANCH" ]]; then
    PAGES_BRANCH="auto-work"
fi

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

if [[ -z "$GITHUB_TOKEN" ]]; then
    echo "Set the GITHUB_TOKEN env variables."
    exit 1
fi

if [[ -z "$GITHUB_HOSTNAME" ]]; then
    GITHUB_HOSTNAME="github.com"
fi

if [[ ! -d "./cache" ]]; then
    mkdir cache
fi

main() {
    echo "Starting deploy..."

    git config --global url."https://".insteadOf git://
    ## $GITHUB_SERVER_URL is set as a default environment variable in all workflows, default is https://github.com
    git config --global url."$GITHUB_SERVER_URL/".insteadOf "git@${GITHUB_HOSTNAME}":

    # needed or else we get 'doubious ...' error
    echo "Disable safe directory check"
    git config --global --add safe.directory '*'
    
    remote_repo="https://${GITHUB_ACTOR}:${GITHUB_TOKEN}@${GITHUB_HOSTNAME}/${TARGET_REPOSITORY}.git"
    remote_branch=$PAGES_BRANCH

    wget -O cache/ithome-rss.xml "https://www.ithome.com/rss"
    wget -O cache/the-paper.json "https://cache.thepaper.cn/contentapi/wwwIndex/rightSidebar"
    wget -O cache/huxiu-rss.xml "https://rss.huxiu.com"

    echo "Pushing artifacts to ${TARGET_REPOSITORY}:$remote_branch"
    git init
    git config user.name "GitHub Actions"
    git config user.email "github-actions-bot@users.noreply.${GITHUB_HOSTNAME}"
    git add .
    git commit -m "update news files in ${TARGET_REPOSITORY}:$remote_branch"
    git push --force "${remote_repo}" main:"${remote_branch}"
    echo "update complete"
}

main "$@"
