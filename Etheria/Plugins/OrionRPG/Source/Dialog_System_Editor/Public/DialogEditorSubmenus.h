#pragma once

#include "CoreMinimal.h"
#include "DialogBuilderEditor.h"

class UToolMenu;

namespace UE::DialogEditor
{
	void ExtendCameraSubmenu(FName InCameraOptionsSubmenuName);
	FToolMenuEntry CreateToolbarCameraSubmenu();

	TSharedRef<SWidget> CreateViewportCameraModeWidget(TWeakPtr<FDialogBuilderEditor> InEditor);
}