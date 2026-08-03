//UCLASS()
//class FPS_BORANAGAJIN_API USaveGameSubsystem
//    : public UGameInstanceSubsystem
//{
//    GENERATED_BODY()
//
//public:
//    bool SaveGame();
//    bool LoadGame();
//
//    bool DoesSaveExist() const;
//    void DeleteSave();
//
//    UFPSGameSave* GetLoadedSave() const
//    {
//        return LoadedSave;
//    }
//
//private:
//    void CapturePlayerData(UFPSGameSave* SaveObject);
//    void RestorePlayerData(const UFPSGameSave* SaveObject);
//
//    void CaptureWorldData(UFPSGameSave* SaveObject);
//    void RestoreWorldData(const UFPSGameSave* SaveObject);
//
//private:
//    UPROPERTY()
//    TObjectPtr<UFPSGameSave> LoadedSave;
//
//    FString SaveSlotName = TEXT("MainSave");
//    int32 UserIndex = 0;
//};