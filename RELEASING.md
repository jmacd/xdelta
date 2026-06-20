# Releasing Xdelta

This document describes how to cut a new Xdelta release.  Releases are
infrequent, so the goal is a short, repeatable checklist that leans on
GitHub Actions to do the heavy lifting.

## Overview

Releases are driven entirely by **annotated git tags** of the form
`vMAJOR.MINOR.PATCH` (for example `v3.2.0`).  Pushing such a tag triggers the
[`Release`](.github/workflows/release.yml) workflow, which:

1. Verifies that the tag matches the version baked into the sources.
2. Builds dependency-free prebuilt binaries for Linux, macOS, and Windows.
3. Assembles a source tarball with `git archive`.
4. Publishes a GitHub Release with auto-generated notes and all of the above
   attached as assets.

## Branch and tag conventions

Xdelta keeps a long-lived **release branch per minor series**, named
`releaseMAJOR_MINOR_apl` (the `_apl` suffix records that these sources are the
Apache-licensed line).  Historically:

- `release3_0_apl` — the 3.0.x series (tags `v3.0.9`, `v3.0.10`, `v3.0.11`).
- `release3_1_apl` — the 3.1.x series (tag `v3.1.0`).

New development happens on `main`.  When a minor series is ready to ship, it is
branched off `main` into a new `releaseMAJOR_MINOR_apl` branch, and point
releases for that series are tagged on that branch.  The 3.2.0 release
introduces `release3_2_apl`.

CI (`.github/workflows/ci.yml`) runs on `main` and on the active release
branches, so add new release branches to its `push:` trigger list when you
create them.

## Versioning

The version lives in two places that must stay in sync; the release workflow
fails fast if the tag disagrees with either:

- `xdelta3/CMakeLists.txt` — the `project(... VERSION x.y.z ...)` line.
- `xdelta3/xdelta3-main.h` — the `Xdelta version x.y.z` string in
  `main_version()`.

## Cutting a new minor release (e.g. 3.2.0)

1. **Make sure `main` is green.**  Wait for CI on `main` to pass.

2. **Create the release branch** from `main`:

   ```sh
   git checkout main && git pull
   git checkout -b release3_2_apl
   ```

3. **Set the version** to `3.2.0` in both files listed under
   [Versioning](#versioning), then commit:

   ```sh
   git commit -am "Release 3.2.0"
   ```

4. **Add the branch to CI.**  In `.github/workflows/ci.yml`, add
   `release3_2_apl` to the `on: push: branches:` list, and commit that too.

5. **Push the branch** and let CI run:

   ```sh
   git push -u origin release3_2_apl
   ```

6. **Tag and push** once CI is green:

   ```sh
   git tag -a v3.2.0 -m "Xdelta 3.2.0"
   git push origin v3.2.0
   ```

7. **Watch the release.**  The `Release` workflow runs on the tag and creates
   the GitHub Release.  Review the auto-generated notes and edit them if
   needed:

   ```sh
   gh run watch
   gh release view v3.2.0 --web
   ```

## Cutting a point release (e.g. 3.2.1)

Point releases stay on the existing series branch:

```sh
git checkout release3_2_apl && git pull
# ... cherry-pick or commit fixes ...
# bump the version to 3.2.1 in the two files above
git commit -am "Release 3.2.1"
git push
# once CI is green:
git tag -a v3.2.1 -m "Xdelta 3.2.1"
git push origin v3.2.1
```

## Re-running a release build

If the release job needs to be re-run for an existing tag (for example after a
transient runner failure), trigger the workflow manually:

```sh
gh workflow run release.yml -f tag=v3.2.0
```

`gh release create` is not idempotent, so delete the partial release first if
one was created:

```sh
gh release delete v3.2.0 --yes
```

## Documentation

User-facing documentation historically lived on the `wiki` branch (for
example `CommandLineSyntax.md`).  That content is being migrated to a GitHub
Pages site; until that lands, the `wiki` branch remains the source of truth.
Keep `README.md` pointing at the canonical documentation location as it moves.
