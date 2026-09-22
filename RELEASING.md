# Releasing Xdelta

All releases are made from `main`. There are no release branches.

- A `v1.*` tag releases the code in `xdelta1/`.
- A `v3.*` tag releases the code in `xdelta3/`.

GitHub Actions builds, checks, packages, and publishes the release after you
push the tag.

## Release checklist

### 1. Update the version

For an **Xdelta 1** release, update the version in:

- `xdelta1/CMakeLists.txt`
- `xdelta1/xdelta.h`
- `xdelta1/xdmain.c`

Also add the release notes to `xdelta1/NEWS`.

For an **Xdelta 3** release, update the version in:

- `xdelta3/CMakeLists.txt`
- `xdelta3/xdelta3-main.h`

### 2. Merge the release commit

Commit the version and release-note changes, open a pull request, and merge it
to `main`. Wait for CI on `main` to pass.

### 3. Tag the commit on `main`

Start from an up-to-date `main`:

```sh
git checkout main
git pull --ff-only
```

For Xdelta 1, replace `1.2.0` with the version being released:

```sh
git tag -a v1.2.0 -m "Xdelta 1.2.0"
git push origin v1.2.0
```

For Xdelta 3, replace `3.2.1` with the version being released:

```sh
git tag -a v3.2.1 -m "Xdelta 3.2.1"
git push origin v3.2.1
```

Pushing the tag starts the correct release workflow. Do not create the GitHub
Release by hand.

### 4. Check the release

Open the Actions and Releases pages:

```sh
gh run list --limit 5
gh release view v1.2.0 --web
```

Use the actual tag in the second command. Xdelta 1 releases are not marked
“Latest”; the newest Xdelta 3 release remains the repository's latest release.

## Re-running a failed release

If the released source is wrong, fix it and make a new version. Never move or
replace a published tag.

If only the workflow failed, re-run the failed jobs on GitHub. You can also run
the workflow again for the existing tag:

```sh
# Xdelta 1
gh workflow run release-v1.yml -f tag=v1.2.0

# Xdelta 3
gh workflow run release.yml -f tag=v3.2.1
```

If a partial GitHub Release was created, delete only the partial release before
re-running the workflow:

```sh
gh release delete v1.2.0 --yes
```
