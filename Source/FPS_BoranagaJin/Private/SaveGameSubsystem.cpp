//bool USaveGameSubsystem::SaveGame()
//{
//    UFPSGameSave* SaveObject =
//        Cast<UFPSGameSave>(
//            UGameplayStatics::CreateSaveGameObject(
//                UFPSGameSave::StaticClass()
//            )
//        );
//
//    if (!SaveObject)
//    {
//        return false;
//    }
//
//    const UWorld* World = GetWorld();
//    if (!World)
//    {
//        return false;
//    }
//
//    SaveObject->SavedLevelName =
//        FName(*World->GetMapName());
//
//    SaveObject->SavedAt = FDateTime::Now();
//
//    CapturePlayerData(SaveObject);
//    CaptureWorldData(SaveObject);
//
//    const bool bSaved =
//        UGameplayStatics::SaveGameToSlot(
//            SaveObject,
//            SaveSlotName,
//            UserIndex
//        );
//
//    if (bSaved)
//    {
//        LoadedSave = SaveObject;
//    }
//
//    return bSaved;
//}