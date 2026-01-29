// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/EnvQueryContext_Target.h"
#include "Enemy/ShooterAIController.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "EnvironmentQuery/EnvQueryTypes.h"

void UEnvQueryContext_Target::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	if (AShooterAIController* Controller = Cast<AShooterAIController>(QueryInstance.Owner))
	{
		if (IsValid(Controller->GetCurrentTarget()))
		{
			UEnvQueryItemType_Actor::SetContextHelper(ContextData, Controller->GetCurrentTarget());
		}
		else
		{
			UEnvQueryItemType_Actor::SetContextHelper(ContextData, Controller);
		}
	}
}
