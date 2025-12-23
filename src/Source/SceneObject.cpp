#include "../Include/SceneObject.h"

/*
SceneObject is a lightweight wrapper that holds:
- Transform data including position, rotation and scale
- Reference to a shared Model
- Override appearance via color or texture for per-instance customization
- Selection state for editor interaction

All methods are currently inline in the header for performance.
If methods grow complex, move implementations here.
*/