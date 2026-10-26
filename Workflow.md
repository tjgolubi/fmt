# Workflow

[Document map](README.md) · [C++ design](CppDesign.md) · [Build and test](BuildAndTest.md) · [Website](Website.md)

Updated October 26, 2026.

## Authorization and scope

Modify project files only when Terry instructs us in the imperative mood.
Questions and statements of intent are discussion, not edit authorization.
Read-only inspection, CI checks, and other actions needed to answer a prompt are
allowed.

Explain proposed changes before editing. An instruction authorizes the work
needed within its scope; it does not authorize unrelated maintenance. Preserve
explicit stopping points such as "stop before committing." Discuss one change,
implement it, verify it, and review it before moving on.

Library and coordination commits, pushes, merges, branch switches, rebases,
deletions, and stashes require authorization for the relevant operation.
Terry normally performs the final commit and push. Do not infer that permission
from authorization to edit or prepare a squash. Website updates follow the
separate publishing procedure in [Website.md](Website.md).

Before edits or branch changes, inspect the actual branch, commit, working tree,
and affected source. Resolve tracked changes from unrelated work with Terry;
do not discard them or stash automatically. Preserve concurrent edits and
re-read a file if it changed while preparing an update.

Untracked convenience scripts, archives, and logs do not make the checkout
unclean. Preserve them; mention them only when they affect the task, may be
included accidentally, or could conflict with a branch switch. Do not include
them in commits or stashes automatically.

Require C++11 compatibility for the entire {fmt}-based release, including all
tests added since `main`. Preserve the older {fmt} tests; do not rename their
identifiers as part of this work. Compatibility notes and dedicated regression
checks may use the former names. Preserve historical changelog terminology.

Keep edits minimal and follow the existing source style. Avoid unrelated
cleanup, declaration changes, or broad toolchain changes. Preserve each existing
file's line endings; create new text files with LF. Git uses
`core.autocrlf=false` and `core.eol=lf`; inspect attributes or overrides before
assuming a checkout follows those settings.

## Repositories and tools

| Repository | Role |
| --- | --- |
| `tjgolubi/fmt` | Library source, tests, user documentation, and fork changelog. Current development branch: `FmtColorDev`. |
| `docs/Coord` in this checkout | Current coordination handoff documents, separate from library development. Earlier history is in `tjgolubi/FmtColorGpt` (`master`). |
| Sites source repository | Separate website implementation, managed by the Sites helper. |

Use **MSYS2 Git** for `fmt` and `FmtColorGpt`: `/usr/bin/git` in MSYS, or
`C:\msys64\usr\bin\git.exe` from PowerShell. Verify executable provenance;
bare `git` in PowerShell may select Codex's bundled native Git. The Sites Node
helper instead needs native Windows Git; see
[Website.md](Website.md#windows-publishing).

Terry removed UCRT64. Do not use or reinstall it for this project. The MSYS PATH
is `/home/tjgolubi/.cargo/bin:/usr/bin:/c/Windows/System32:/c/Windows`.
Git-cliff 2.14.2 is the MSYS/Cygwin build at
`/home/tjgolubi/.cargo/bin/git-cliff.exe`, built without default features for
`x86_64-pc-cygwin`; it does not require UCRT64.

For SSH in a nonlogin MSYS shell, source `~/.ssh/agent.env`. Reuse the existing
agent, keys, Git identity, and SSH configuration; do not replace them to solve
an environment problem. Use the GitHub connector for remote inspection when
appropriate. Keep credentials out of files, output, and handoffs.

The library remotes are `origin` → `tjgolubi/fmt` and `upstream` → `fmtlib/fmt`.
Keep fork changes on development branches and `main` aligned with upstream.

## Branches and integration

The current baseline is recorded in [README.md](README.md#current-baseline).
Do not continue from an older branch merely because it appears in historical
documents. Verify refs and ancestry before changing history.

Create focused branches when implementation begins from the agreed baseline.
Record the proposed name first. Use lowercase slash prefixes and PascalCase
subjects, such as `feat/Fluent`, `fix/Termux`, or `doc/ColorGuide`; other suitable
prefixes are allowed. A name or design entry neither creates a branch nor grants
approval to implement it or include it on the website.

Each accepted feature integration is one squash commit containing applicable
source, tests, documentation, and changelog changes. Do not substitute a
fast-forward or ordinary merge. Accepted feature commits and publication fixes are recorded in the current
baseline; this policy does not authorize rewriting them.

### CI gate and acceptance

1. Confirm scope, baseline, dependencies, and preservation of local work.
2. Review the complete candidate. Assess source/API, binary/ABI, and stored-data
   compatibility separately. Unchanged size does not prove ABI compatibility.
3. Finish applicable source, tests, and user documentation. Run the relevant
   local checks.
4. Terry pushes the candidate, or explicitly authorizes the assistant to push.
   Verify local and GitHub full SHAs match. **Every applicable workflow and
   matrix job must pass for that exact SHA before squash preparation.** Queued,
   running, failed, cancelled, or missing expected jobs are not passes.
   Creating or pushing a branch at an unchanged baseline tests that baseline,
   not uncommitted feature files. Check that the feature itself is in the pushed
   tree before treating a green run as candidate validation.
5. Obtain authorization for the squash onto the agreed development baseline.
   Prepare it, verify the resulting tree and relevant integration checks, and
   stop uncommitted for Terry's review unless instructed otherwise.
   A squash stages its changes: review with `git diff --cached`; plain
   `git diff` shows only unstaged changes. `git commit -C <candidate>` reuses
   the candidate's message and authorship without an editor; lowercase `-c`
   opens the configured editor to revise that message.
6. Terry commits the squash. Run git-cliff over the accepted history, starting
   at `main` and ending at that actual commit, inspect the generated changelog,
   and leave it for review.  Terry stages it and uses
   `git commit --amend --no-edit` to include it in the same squash commit.
   If the message needs editing, amend it first, regenerate the changelog, then
   amend again with the generated file.
7. Terry pushes the final amended commit, or explicitly authorizes the assistant
   to push. Verify the intended commit structure, matching local/GitHub full
   SHAs, and every applicable CI job for that final SHA, including guide
   publication on `FmtColorDev`. Candidate results do not replace final
   integration checks.
8. Publish an authorized website update before recording its confirmed
   publication in coordination documents. Update or close an issue only when
   instructed; a successful fix does not by itself authorize closure.

Any amendment, new commit, rebase, changed baseline, or changed squash tree needs
fresh CI for the resulting SHA. Never rely on earlier validation for new code.
The four ordinary push workflows are `linux`, `windows`, `macos`, and `doc`.
Pull-request-only checks such as `lint` and `fuzz` must pass when applicable;
push results do not establish those passes. Do not trigger unrelated release,
security, or publication workflows merely to manufacture a broader result.

Record intentional skips and exclusions explicitly. Linux fail-fast is enabled
on `FmtColorDev`; its GCC 16/C++26 reflection exclusion remains tracked in
[issue #2](https://github.com/tjgolubi/fmt/issues/2). The separate upstream
compatibility branch includes that job and has passed it. Results from one
branch do not restore disabled coverage on another.

## Commit messages and changelog

Use Conventional Commits compatible with `cliff.toml`, for example
`feat(color): named RGB constants` or `fix(docs): publish the user guide`.
Mark deliberate breaks with `!` and a `BREAKING CHANGE:` footer. Describe actual
effects without promising stable numeric or binary representations.

Maintain `FmtColorChangeLog.md` separately from upstream `ChangeLog.md`.
Keep entries concise; detailed rationale belongs on the website. Coordination
revisions are not library release entries. Preserve explicit decisions to omit
an entry for a particular fix rather than adding one later without instruction.

Use git-cliff **2.14.2** and an explicit accepted-history range after the
upstream branch-off point. Exclude upstream history and intermediate feature commits.
After Terry commits the squash on `FmtColorDev`, generate the changelog with:

```bash
git-cliff --offline --no-exec --output FmtColorChangeLog.md main..HEAD
```

The baseline is excluded. `main..HEAD` is a convenient equivalent only while
`main` still points to that exact upstream baseline. `cliff.toml` is discovered
automatically; its baseline comment can lag the current branch, so verify the
range. `--offline` prevents network metadata requests and `--no-exec` disables
external processing commands. Use the generated file, review its diff, and let
Terry stage and amend it into the squash before pushing. Do not manually rewrite
entries: adjust the template or relevant commit message when needed, then
regenerate. Do not include intermediate feature commits or invent a prospective
hash. This commit/generate/amend/push sequence was verified with VarColor,
Palette, and Ostream.

The current template groups entries by commit type and orders them oldest first
within each group. It uses the subject, scope, breaking marker, and
`BREAKING CHANGE:` description; it does not reproduce the ordinary commit body
or summarize it automatically. Use Markdown backticks around identifiers in
commit messages so generated entries retain code formatting. Keep subjects and
breaking-change descriptions readable. Changing an already published message
rewrites history and requires separate authorization and coordination.

Keep bookkeeping exclusions before general parsers, protect breaking commits,
retain deterministic groups, and fail on unexpected nonconventional commits.
The `fmtcolor-` tag namespace separates fork tags from imported upstream tags;
tag filtering alone does not select the correct history.

Do not automatically bump inherited `FMT_VERSION` or assign a fork release
number. Fork versioning remains undecided. Record the exact upstream baseline;
a development header's version is not proof of a tagged release. Push only
authorized tags; published release tags stay fixed.

## Upstream and cross-machine updates

`test/UpstreamCompatibility` applies the enhanced header to the upstream
baseline and records the exact upstream test/documentation adjustments in
`UpstreamTestCompatibility.md`. It is a verification branch, not a second
feature-development line. Preserve upstream identifier spellings in its tests.
Do not update upstream `ChangeLog.md` merely for compatibility verification;
release-note requirements can be settled when preparing an upstream PR.

Before citing a green run, verify that the enhanced header and adjustments
are committed and pushed, not merely present in the working tree. A push of
an unchanged baseline tests upstream code only. Match full local/remote/run
SHAs and inspect every applicable job. Terry plans periodic rebases of this
branch; rebasing or refreshing its header requires authorization and fresh CI.

Keeping production changes in `color.h` reduces rebase conflicts, but changes
to supporting APIs, tests, documentation, or tooling can still require fixes.
The original upstream baseline remains `0fc31633`; Terry reported upstream/main
had not advanced at this handoff. Inspect current refs before any later rebase.
Eventual consolidation of the complete contribution is a future upstream
submission step, not permission to rewrite current refs.

Continue on the agreed upstream baseline. Inspect upstream changes before
incorporating them. Update the development baseline when a relevant upstream
change warrants it, and before offering the completed extension upstream;
do not rebase unrelated work merely because upstream advanced.

A fast-forward update of `main` is separate from rebasing development branches:

```bash
git fetch upstream
git switch main
git merge --ff-only upstream/main
git push origin main
```

These commands are instructions for an authorized update, not standing
permission to run them. Check local changes and coordinate branch switches.

Before rewriting published history, record the old tip and coordinate with
other checkouts. Resolve conflicts deliberately, compare content, rerun checks,
and use an authorized `--force-with-lease` rather than unguarded force.
A lease protects the expected remote ref, not the intended content. Inspect
refs and reflogs before synchronizing a second machine; never force-push older
history simply to make a rejected push succeed. Refresh website commit links
after an authorized rewrite.

## Coordination maintenance

These documents hand off context and design vision to the next task
conversation. Maintain a coherent account of the current state, accepted
behavior, constraints, and open decisions. Replace superseded statements;
do not append a chronological log of commits, fixes, or conversations. Keep
only the provenance needed to identify the current baseline and validation.

Read these documents before new work and state when required material is
unavailable or stale. Update affected decisions without maintaining competing
authoritative copies in attachments. Keep proposals, implemented behavior,
tested results, and acceptance distinct.

When asked to update both website and coordination, publish and verify the
website first. Then record the actual version, source, and deployment result.
Report whether coordination edits are local, committed, or pushed; another
machine cannot read local changes from GitHub until they are pushed.
