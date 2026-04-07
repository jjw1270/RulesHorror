// Copyright (c) 2026 장윤제. All rights reserved.


#include "RulesHorrorItemDefines.h"


#if WITH_EDITOR
EDataValidationResult FStoryTableRow::IsDataValid(FDataValidationContext& _context) const
{
	return Super::IsDataValid(_context);
}
#endif
