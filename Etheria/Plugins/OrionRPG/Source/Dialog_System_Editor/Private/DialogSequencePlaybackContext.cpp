#include "DialogSequencePlaybackContext.h"
#include "SDialogPreviewViewport.h"
#include "DialogSequence.h"
#include "DialogBuilderEditor.h"

#include "Editor.h"
#include "Engine/Engine.h"
#include "Widgets/SNullWidget.h"

FDialogSequencePlaybackContext::FDialogSequencePlaybackContext(UDialogSequence* InDialogSequence)
	: DialogSequence(InDialogSequence)
{
	FEditorDelegates::MapChange.AddRaw(this, &FDialogSequencePlaybackContext::OnMapChange);
	FEditorDelegates::PreBeginPIE.AddRaw(this, &FDialogSequencePlaybackContext::OnPieEvent);
	FEditorDelegates::BeginPIE.AddRaw(this, &FDialogSequencePlaybackContext::OnPieEvent);
	FEditorDelegates::PostPIEStarted.AddRaw(this, &FDialogSequencePlaybackContext::OnPieEvent);
	FEditorDelegates::PrePIEEnded.AddRaw(this, &FDialogSequencePlaybackContext::OnPieEvent);
	FEditorDelegates::EndPIE.AddRaw(this, &FDialogSequencePlaybackContext::OnPieEvent);

	if (GEngine)
	{
		GEngine->OnWorldAdded().AddRaw(this, &FDialogSequencePlaybackContext::OnWorldListChanged);
		GEngine->OnWorldDestroyed().AddRaw(this, &FDialogSequencePlaybackContext::OnWorldListChanged);
	}

	RefreshDialogContextObject();
}

FDialogSequencePlaybackContext::~FDialogSequencePlaybackContext()
{
	FEditorDelegates::MapChange.RemoveAll(this);
	FEditorDelegates::PreBeginPIE.RemoveAll(this);
	FEditorDelegates::BeginPIE.RemoveAll(this);
	FEditorDelegates::PostPIEStarted.RemoveAll(this);
	FEditorDelegates::PrePIEEnded.RemoveAll(this);
	FEditorDelegates::EndPIE.RemoveAll(this);

	if (GEngine)
	{
		GEngine->OnWorldAdded().RemoveAll(this);
		GEngine->OnWorldDestroyed().RemoveAll(this);
	}
}


void FDialogSequencePlaybackContext::SetDialogViewport(TSharedPtr<SDialogPreviewViewport> InDialogViewport)
{
	DialogViewport = InDialogViewport;

	if (DialogContextObject.IsValid())
	{
		DialogContextObject->SetDialogViewport(DialogViewport);
	}
}

TArray<UObject*> FDialogSequencePlaybackContext::GetEventContexts() const
{
	TArray<UObject*> Result;

	if (UObject* ContextObject = GetPlaybackContextAsObject())
	{
		Result.Add(ContextObject);
	}

	return Result;
}

UObject* FDialogSequencePlaybackContext::GetPlaybackContextAsObject() const
{
	FDialogSequencePlaybackContext* MutableThis = const_cast<FDialogSequencePlaybackContext*>(this);
	MutableThis->RefreshDialogContextObject();

	if (DialogContextObject.IsValid())
	{
		return DialogContextObject.Get();
	}

	return nullptr;
}

IMovieScenePlaybackClient* FDialogSequencePlaybackContext::GetPlaybackClientAsInterface() const
{
	// If needed later, return a real playback client object here.
	return nullptr;
}


TSharedRef<SWidget> FDialogSequencePlaybackContext::BuildWorldPickerCombo()
{
	// Optional UI; keep null widget for now.
	return SNullWidget::NullWidget;
}

void FDialogSequencePlaybackContext::OnMapChange(uint32)
{
	DialogContextObject = nullptr;
	
}

void FDialogSequencePlaybackContext::OnPieEvent(bool)
{
	DialogContextObject = nullptr;
	
}

void FDialogSequencePlaybackContext::OnWorldListChanged(UWorld*)
{
	DialogContextObject = nullptr;
	
}

UObject* FDialogSequencePlaybackContext::ResolvePlaybackContextOuter() const
{

	if (GEngine)
	{
		UWorld* EditorWorld = nullptr;

		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::PIE && Context.World() != nullptr)
			{
				return Context.World();
			}

			if (Context.WorldType == EWorldType::Editor)
			{
				EditorWorld = Context.World();
			}
		}

		if (EditorWorld)
		{
			return EditorWorld;
		}
	}

	if (GEditor)
	{
		return GEditor->GetEditorWorldContext().World();
	}

	return nullptr;
}

void FDialogSequencePlaybackContext::RefreshDialogContextObject()
{
	UObject* OuterObject = ResolvePlaybackContextOuter();

	if (!OuterObject)
	{
		OuterObject = GetTransientPackage();
	}


	if (DialogContextObject.IsValid() && DialogContextObject->GetOuter() == OuterObject)
	{
		DialogContextObject->SetDialogViewport(DialogViewport);
		return;
	}

	DialogContextObject.Reset();
	DialogContextObject = TWeakObjectPtr<UDialogPlaybackContextObject>(NewObject<UDialogPlaybackContextObject>(OuterObject));

	if (DialogContextObject.IsValid())
	{
		DialogContextObject->SetDialogViewport(DialogViewport);
	}
}

void UDialogPlaybackContextObject::SetDialogViewport(TWeakPtr<SDialogPreviewViewport> InDialogViewport)
{
	DialogViewport = InDialogViewport;
}

void UDialogPlaybackContextObject::AddDialogToViewport(UUserWidget* InWidget)
{
	if (TSharedPtr<SDialogPreviewViewport> Pinned = DialogViewport.Pin())
	{
		Pinned->AddDialogOverlayWidget(InWidget);
	}
}

void UDialogPlaybackContextObject::RemoveDialogFromViewport()
{
	if (TSharedPtr<SDialogPreviewViewport> Pinned = DialogViewport.Pin())
	{
		Pinned->RemoveDialogOverlayWidget();
	}
}

AActor* UDialogPlaybackContextObject::GetDialogDefinitionActor(UDialogDefinition* InDialogDefinition)
{
	if (!InDialogDefinition)
	{
		return nullptr;
	}

	if (TSharedPtr<SDialogPreviewViewport> PinnedViewport = DialogViewport.Pin())
	{
		if (TSharedPtr<FDialogBuilderEditor> DialogEditor = PinnedViewport->GetDialogEditor())
		{
			return DialogEditor->GetDialogDefinitionActor(InDialogDefinition);
		}
	}

	return nullptr;
}