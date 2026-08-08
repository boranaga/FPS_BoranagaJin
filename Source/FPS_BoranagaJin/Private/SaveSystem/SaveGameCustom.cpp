// Fill out your copyright notice in the Description page of Project Settings.


#include "SaveSystem/SaveGameCustom.h"

UFPSGameSave::UFPSGameSave()
{
	SaveVersion = 1;
	SavedAt = FDateTime::Now();
}
