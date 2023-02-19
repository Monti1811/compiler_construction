#pragma once

#include "types/type.h"

#include "types/function.h"
#include "types/struct.h"
#include "types/pointer.h"

std::optional<TypePtr> unifyTypes(TypePtr, TypePtr);
