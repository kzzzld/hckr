# Releasing hckr

Releases are source archives built from stable version tags. No credentials or additional secrets need to be configured: each repository uses its own GitHub Actions token.

## Publish a version

Choose a new `vMAJOR.MINOR.PATCH` tag. For example, when ready to publish the first release:

```sh
git switch main
git pull --ff-only
git tag -a v0.1.0 -m "hckr v0.1.0"
git push origin v0.1.0
```

The `Test and release` workflow builds and tests on macOS and Linux. Only after both pass, it creates a GitHub Release with generated notes and these assets:

- `hckr-0.1.0.tar.gz`: the source tree, including its INI themes.
- `SHA256SUMS`: the archive's SHA-256 checksum.

Archives are reproducible for the same tag. Rerunning publication verifies an existing release instead of replacing its assets. Use a new version for corrections; do not move published tags or replace release archives. Pre-release tags are not supported by this stable-release workflow.

## Recover an existing tag without a release

A tag is not itself a GitHub Release. A successful tap check can make no changes if no stable release exists. GitHub also skips tag-push workflows when the tagged commit message contains markers such as `[skip ci]`.

Do not delete or move an existing version tag. Publish it through the manual release action instead:

```sh
gh workflow run release.yml --repo kzzzld/hckr --ref main -f tag=v0.1.0
gh run list --repo kzzzld/hckr --workflow release.yml
```

The manual action tests and packages the exact existing tag. Leaving its `tag` input empty runs tests only. After the release finishes, run the tap updater below. When viewing its run, specify `--repo kzzzld/tap`; otherwise `gh` uses the repository in your current directory.

## Update Homebrew

The `Update hckr` workflow in [kzzzld/tap](https://github.com/kzzzld/tap) checks for a stable release hourly. It downloads the archive, verifies `SHA256SUMS`, and commits the versioned URL and checksum to `Formula/hckr.rb`. It preserves the development `head` source. It does nothing before the first release or when the formula is already current, and refuses downgrades.

To request an immediate check after the release workflow finishes:

```sh
gh workflow run update-hckr.yml --repo kzzzld/tap
```

You can also select **Actions → Update hckr → Run workflow** in the tap repository. The hourly schedule runs on its default branch; GitHub may delay scheduled runs or disable them after prolonged repository inactivity. Manual dispatch is the fallback.

After the first release, users can install the stable version:

```sh
brew tap kzzzld/tap https://github.com/kzzzld/tap
brew install kzzzld/tap/hckr
```

To switch from an existing `--HEAD` installation after the first stable release, run `brew uninstall hckr` followed by `brew install kzzzld/tap/hckr`. Your personal `~/.config/hckr` files are separate from the Homebrew package. Afterwards, `brew update` and `brew upgrade kzzzld/tap/hckr` install stable updates. `brew install --HEAD kzzzld/tap/hckr` remains available for development builds.

## Maintain the formula

`Formula/hckr.rb` in the application repository is the development template. The tap's copy additionally contains the current stable URL and checksum. When changing build steps or dependencies, update both copies while preserving the tap's stable source. The release updater changes only `url` and `sha256`.

No binaries or Homebrew bottles are published by this system. Homebrew builds from the versioned source archive and installs the bundled themes in its shared-data directory.
