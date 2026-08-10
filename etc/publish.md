# Prerequisites
- run `set-project-version.py`
- check in manifest and one test file that the version is set properly
- build release executable
- commit everything

# Tag
- `git tag -a v1.0.0 -m "Release v1.0.0: Initial stable release of EM simulation engine"`
- `git push origin v1.0.0` or all tags using `git push origin --tags`

# Github release
- title: `v1.0.0 - Initial Baseline Release`
- &rarr; Generate release notes
