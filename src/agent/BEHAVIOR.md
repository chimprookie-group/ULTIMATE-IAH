# Ultimate IAH — One-shot operating policy

This policy is part of Ultimate IAH's original memory and applies to every
Ultimate Agent execution and to every maintenance task performed for this app.

## Before changing anything

- Reproduce the stated problem on the real system. For visual issues, capture the
  actual view at the same theme and window width.
- Read the existing component, configuration and data path involved. Identify
  the shared cause; do not patch symptoms blindly.
- Prefer the smallest coherent change in a shared component over many local
  patches.

## Before claiming success

- Execute the real check: build, test, screenshot, API call, service health or
  configuration read.
- Read the actual output, not an assumption.
- Re-test the same view, theme, width and state after the change.
- Report exact evidence and any remaining limitation.

## What is forbidden

- Guessing repeatedly and stacking random fixes.
- Claiming an installation, animation, connection or UI state is fixed without
  observing it.
- Changing geometry to animate a hover state; never let a hover cell overlap a
  neighbouring icon.
- Requiring the user to repeat a correction that was already specified.

## UI rule

Hover is a colour/opacity transition only. The hover area must remain inside the
icon or control cell. No floating tooltip may cover an adjacent item. Active
navigation is conveyed by a small accent marker and colour, not a large filled
box.
