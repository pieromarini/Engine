#include "core/core_inc.h"
#include "platform/os/core/os_core.h"
#include "platform/os/gfx/os_gfx.h"
#include "platform/os/os_inc.h"
#include "platform/render/render_inc.h"

#include "core/core_inc.cpp"
#include "platform/os/os_inc.cpp"
#include "platform/render/render_inc.cpp"

struct Camera {
  vec3 velocity;
  vec3 position;
  f32 pitch;
  f32 yaw;
  f32 fov;
};

struct Window {
  Window* next;
  Window* prev;
  OSWindowHandle handle;

  Camera* camera;
};

struct State {
  Arena* arena;
  Window* firstWindow;
  Window* lastWindow;

  u64 t0;
  f32 osTimerFreq;

  b32 quit;
};

per_thread State* state;

void processEvents(Window* window, OS_EventList* events) {
  for (OS_Event* event = events->first, *next = nullptr; event != nullptr; event = next) {
    next = event->next;
    Camera* camera = window->camera;

    if (event->kind == OS_EventKind_Press) {
      if (event->key == OS_Key_W) {
        camera->velocity.z = 1;
      }
      if (event->key == OS_Key_S) {
        camera->velocity.z = -1;
      }
      if (event->key == OS_Key_A) {
        camera->velocity.x = -1;
      }
      if (event->key == OS_Key_D) {
        camera->velocity.x = 1;
      }

      if (event->key == OS_Key_M) {
        OS_setRelativeMouseMode(window->handle, !OS_getRelativeMouseMode(window->handle));
        OS_consumeEvent(events, event);
      }
    }

    if (event->kind == OS_EventKind_Release) {
      if (event->key == OS_Key_W) {
        camera->velocity.z = 0;
      }
      if (event->key == OS_Key_S) {
        camera->velocity.z = 0;
      }
      if (event->key == OS_Key_A) {
        camera->velocity.x = 0;
      }
      if (event->key == OS_Key_D) {
        camera->velocity.x = 0;
      }
    }

    if (OS_getRelativeMouseMode(window->handle) && event->kind == OS_EventKind_MouseMove) {
      camera->yaw += event->relPosition.x / 200.0f;
      camera->pitch += event->relPosition.y / 200.0f;

      OS_consumeEvent(events, event);
    }
  }
}

void update() {
  // convert ticks to microseconds
  f32 deltaTime = (f32)((OS_readOSTimer() - state->t0) * 1000) / state->osTimerFreq;
  state->t0 = OS_readOSTimer();

  Temp scratch = ScratchBegin();
  OS_EventList events = OS_getEvents(scratch.arena);

  for (Window* window = state->firstWindow; window != nullptr; window = window->next) {
    Region2D rect = OS_clientRectFromWindow(window->handle);
    vec2 size = region2DSize(rect);
    Render_startWindow(window->handle, size);

    if (OS_windowIsMinimized(window->handle)) {
      continue;
    }

    processEvents(window, &events);

    Render_setViewMatrix(window->camera->position, window->camera->pitch, window->camera->yaw);
    Render_setProjectionMatrix(window->camera->fov, size.x / size.y, 10000.0f, 0.1f);

    // Camera movement
    mat4 cameraRotation = matrixMakeRotation(window->camera->pitch, window->camera->yaw);
    vec3 movement = window->camera->velocity * 0.01f * deltaTime;
    window->camera->position = window->camera->position + vecTransform(vec4{ movement.x, movement.y, movement.z, 0.f }, cameraRotation).xyz;

    Render_update();

    Render_endWindow(window->handle);
  }

  for (OS_Event* event = events.first, *next = nullptr; event != nullptr; event = next) {
    next = event->next;
    if (event->kind == OS_EventKind_WindowClose) {
      OSWindowHandle window = event->window;
  
      OS_destroyWindow(window);

      OS_consumeEvent(&events, event);

      state->quit = true;
    }
  }

  ScratchEnd(scratch);
}

void entryPoint() {
  auto arena = ArenaAllocDefault();
  state = PushStruct(arena, State);
  state->arena = arena;

  OSWindowHandle osWindow = OS_createWindow(0, vec2{ 1920, 1080 }, Str8L("Engine"));

  Render_equipWindow(osWindow);

  OS_windowFirstPaint(osWindow);
  OS_windowSetRepaint(osWindow, update);

  Window* window = PushStruct(arena, Window);
  window->handle = osWindow;
  window->camera = PushStruct(arena, Camera);
  window->camera->fov = 70.0f;
  window->camera->position = { 0.0f, 20.0f, 20.0f };

  state->osTimerFreq = (f32)OS_getOSTimerFreq();

  DLLPushBack(state->firstWindow, state->lastWindow, window);

  // Render_loadScene(Str8L("../res/models/bistro/bistro.glb"));
  Render_loadScene(Str8L("../res/models/sponza-optimized/Sponza.gltf"));

  while(!state->quit) {
    update();
  }

  arenaRelease(arena);
}
