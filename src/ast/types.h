#pragma once

#include "types/type.h"

#include "types/function.h"
#include "types/pointer.h"
#include "types/struct.h"

std::optional<TypePtr> unifyTypes(TypePtr, TypePtr);
