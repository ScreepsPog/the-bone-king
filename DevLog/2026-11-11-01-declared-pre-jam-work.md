# Declared pre-jam work (2026-11-10 01:36 PST to 2026-11-11 15:13 PST)

Everything below was made before the jam opened. It is tool and pipeline setup. No game mechanic, level, or asset for
the entry existed before November 12; the only level packaged in rehearsals was the engine's empty `Template_Default`.

## Asked

Set up an Unreal Engine 5.9 Preview project that can ship a game to the browser, with AI agents able to drive the
editor, and prove the package pipeline before the jam.

## What the agents did

| Commit | When (PST) | What it is |
|---|---|---|
| `631bbc2` | 2026-11-10 01:36 | Project snapshot, git and LFS setup, ignore rules. |
| `5226808` | 2026-11-10 01:37 | Empty C++ game module (from the engine's blank template; the web needs a C++ project), AI toolset plugins enabled for the editor only, Claude Test Helper plugin (input and state helpers for automated tests). |
| `f97f3ae` | 2026-11-10 01:55 | Web build config: cook the empty template map, keep `ToolsetRegistry` enabled on the web platform. |
| `ee9caf2` | 2026-11-10 01:56 | Project notes for agents: working rules, web build facts, package and serve commands. |
| `720020a`, `6c022b8` | 2026-11-11 14:27, 14:33 | Design spec for jam readiness. |
| `ed182a2` | 2026-11-11 14:35 | Implementation plan for jam readiness. |
| `9889724`, `ffd6ad5` | 2026-11-11 14:40, 14:58 | `Tools/Jam/jam_checks.py`: checks renderer settings, package size limits, and zip layout. |
| `082ef9d`, `e34bf0d`, `740c3f8` | 2026-11-11 15:03 to 15:13 | Renderer settings (Lumen, virtual shadow maps, ray tracing, Substrate off; mobile clustered reflections on), rehearsal packages for web and Windows, this dev log. |

Also set up outside the repo: emscripten 6.0.10, and the unreal-toolkit and superpowers plugins for Claude Code.

## What failed and the fix

- The first web cooks failed on six generated `ToolsetRegistry` assets. The engine's deploy skill says to deny that
  plugin on the web; the engine's getting-started guide says to leave it on. Leaving it on fixed the cook.
- The toolkit's command for describing editor tools did not work on 5.9 because the editor's tool names changed. A
  small probe script was used instead.
- The first reading of the jam rules missed that web builds are an accepted entry format. The director caught it, and
  the plan changed to make the web build the entry with a Windows zip as the fallback.

## Screenshots

None kept from setup.

## Time spent

About one working day across two sessions.
