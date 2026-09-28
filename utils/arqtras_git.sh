#!/usr/bin/env bash
# Run git/gh as ArqTras for commits, pushes, and releases.
# Usage:
#   utils/arqtras_git.sh commit ...
#   utils/arqtras_git.sh push ...
#   utils/arqtras_git.sh release-create <tag> [notes-file]
#   utils/arqtras_git.sh release-recreate <tag> [notes-file]
#
# Requires a valid ArqTras PAT in ARQTRAS_GITHUB_TOKEN or GH_ACTIONS_PAT
# (classic token, repo scope). Cloud Agent App tokens always publish as cursor[bot].

set -euo pipefail

ARQ_NAME="ArqTras"
ARQ_EMAIL="33489188+ArqTras@users.noreply.github.com"
TOKEN="${ARQTRAS_GITHUB_TOKEN:-${GH_ACTIONS_PAT:-}}"

export GIT_AUTHOR_NAME="$ARQ_NAME"
export GIT_AUTHOR_EMAIL="$ARQ_EMAIL"
export GIT_COMMITTER_NAME="$ARQ_NAME"
export GIT_COMMITTER_EMAIL="$ARQ_EMAIL"

die() { echo "arqtras_git: $*" >&2; exit 1; }

token_ok() {
  [[ -n "$TOKEN" ]] || return 1
  [[ ${#TOKEN} -ge 20 ]] || return 1
  [[ "$TOKEN" == ghp_* || "$TOKEN" == github_pat_* || "$TOKEN" == gho_* ]] || return 1
  GH_TOKEN="$TOKEN" gh api user --jq .login 2>/dev/null | grep -qiE '^(ArqTras)$'
}

require_token() {
  token_ok || die "need a valid ArqTras PAT in ARQTRAS_GITHUB_TOKEN (or GH_ACTIONS_PAT). Current token missing/invalid — push/publish would stay cursor[bot]."
}

cmd="${1:-}"
shift || true

case "$cmd" in
  commit)
    git -c user.name="$ARQ_NAME" -c user.email="$ARQ_EMAIL" commit "$@"
    ;;
  push)
    require_token
    git -c "http.https://github.com/.extraheader=AUTHORIZATION: basic $(printf 'x-access-token:%s' "$TOKEN" | base64 -w0)" push "$@"
    ;;
  release-create)
    require_token
    tag="${1:?tag required}"
    notes="${2:-}"
    if [[ -n "$notes" ]]; then
      GH_TOKEN="$TOKEN" gh release create "$tag" --repo PrivacyDataCoin-Project/PDC --title "PDC $tag" --notes-file "$notes"
    else
      GH_TOKEN="$TOKEN" gh release create "$tag" --repo PrivacyDataCoin-Project/PDC --title "PDC $tag" --generate-notes
    fi
    ;;
  release-recreate)
    require_token
    tag="${1:?tag required}"
    notes="${2:-}"
    GH_TOKEN="$TOKEN" gh release delete "$tag" --repo PrivacyDataCoin-Project/PDC --yes 2>/dev/null || true
    if [[ -n "$notes" ]]; then
      GH_TOKEN="$TOKEN" gh release create "$tag" --repo PrivacyDataCoin-Project/PDC --title "PDC $tag" --notes-file "$notes"
    else
      GH_TOKEN="$TOKEN" gh release create "$tag" --repo PrivacyDataCoin-Project/PDC --title "PDC $tag" --generate-notes
    fi
    ;;
  check-token)
    if token_ok; then
      echo "OK: token authenticates as $(GH_TOKEN="$TOKEN" gh api user --jq .login)"
    else
      echo "FAIL: no usable ArqTras PAT (push/publish will be cursor[bot])"
      exit 1
    fi
    ;;
  *)
    die "usage: $0 {commit|push|release-create|release-recreate|check-token} ..."
    ;;
esac
