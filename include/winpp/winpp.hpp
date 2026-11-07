// WinAPI++ 总入口
//
// 用户只需 #include <winpp/winpp.hpp>。
#pragma once

#include <winpp/compat/config.hpp>
#include <winpp/compat/export.hpp>
#include <winpp/compat/shims.hpp>

#include <winpp/core/error.hpp>
#include <winpp/core/result.hpp>
#include <winpp/core/check.hpp>
#include <winpp/core/handle.hpp>
#include <winpp/core/traits.hpp>
#include <winpp/core/string.hpp>

#include <winpp/fs/file.hpp>
#include <winpp/fs/path.hpp>

#include <winpp/sys/process.hpp>
#include <winpp/sys/registry.hpp>
#include <winpp/sys/env.hpp>
#include <winpp/sys/time.hpp>

#include <winpp/ui/window.hpp>
#include <winpp/ui/dpi.hpp>
#include <winpp/ui/controls.hpp>
#include <winpp/ui/dialog.hpp>
#include <winpp/ui/tray.hpp>
#include <winpp/ui/clipboard.hpp>

#include <winpp/gdi/dc.hpp>
#include <winpp/gdi/objects.hpp>

#include <winpp/com/com_ptr.hpp>
