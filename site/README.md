# Xdelta documentation site

This directory contains the [MkDocs](https://www.mkdocs.org/) +
[Material](https://squidfunk.github.io/mkdocs-material/) source for the Xdelta
documentation site published at <https://jmacd.github.io/xdelta/>.

The pages here were migrated from the project's old `wiki` branch and updated
for the current 3.2.x sources (armor mode, liblzma, `merge`/`recode`, and the
Apache 2.0 license).

## Build and preview locally

```sh
cd site
python -m venv .venv && . .venv/bin/activate   # optional
pip install -r requirements.txt
mkdocs serve        # live preview at http://127.0.0.1:8000/
mkdocs build        # static output in site/_site/
```

## Deployment

The site is built and deployed to GitHub Pages by
[`.github/workflows/site.yml`](../.github/workflows/site.yml) on every push to
`main` that touches `site/`.
