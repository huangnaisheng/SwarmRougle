// SRGameModeBase.cpp
#include "SRGameModeBase.h"
#include "Engine/Engine.h"          
#include "Engine/AssetManager.h"
#include "SRGameStateBase.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

void ASRGameModeBase::HandlePlayerDeath(AController* PlayerController)
{
    if (!PlayerController) return;

    // 1. 获取当前的肉体 (Character) 并销毁或播放死亡表现
    ACharacter* DeadCharacter = Cast<ACharacter>(PlayerController->GetPawn());
    if (DeadCharacter)
    {
        // 剥夺控制权
        PlayerController->UnPossess();
        // 销毁肉体 (或者改为布娃娃系统延时销毁)
        DeadCharacter->Destroy();
    }

    // 2. 开启复活倒计时
    // 这里可以启动一个 Timer，比如 5 秒后调用 RespawnPlayer(PlayerController)
    FTimerHandle RespawnTimerHandle;
    GetWorldTimerManager().SetTimer(RespawnTimerHandle, [this, PlayerController]()
    {
        this->RespawnPlayer(PlayerController);
    }, 5.0f, false);
}

void ASRGameModeBase::RespawnPlayer(AController* PlayerController)
{
    if (!PlayerController) return;

    // 获取出生点 (这里简化为直接拿基础逻辑的出生点)
    AActor* PlayerStart = ChoosePlayerStart(PlayerController);

    // 1. 生成新的肉体 (Character)
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    ACharacter* NewCharacter = GetWorld()->SpawnActor<ACharacter>(DefaultPawnClass, PlayerStart->GetActorTransform(), SpawnParams);

    if (NewCharacter)
    {
        // Controller Possess 时，会触发我们之前写在 Character 里的 PossessedBy 函数。
        // 在 PossessedBy 里，会调用 InitAbilityActorInfo！
        // 一瞬间，新的肉体就完美接管了含有满级属性和所有 Tag 的旧 ASC。
        PlayerController->Possess(NewCharacter);
    }
}

void ASRGameModeBase::StartNextWave()
{
    ASRGameStateBase* SRGameState = GetGameState<ASRGameStateBase>();
    if (SRGameState)
    {
        // 推进波次，GameState 会自动把新波次同步给所有客户端
        SRGameState->SetCurrentWave(SRGameState->CurrentWave + 1);

        // 此处可以触发你的 Enemy Spawner 系统开始工作
    }
}


