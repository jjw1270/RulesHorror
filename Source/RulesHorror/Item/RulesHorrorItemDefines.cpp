// Copyright Epic Games, Inc. All Rights Reserved.


#include "RulesHorrorItemDefines.h"


#if WITH_EDITOR
EDataValidationResult FStoryTableRow::IsDataValid(FDataValidationContext& _context) const
{
	return Super::IsDataValid(_context);
}
#endif
