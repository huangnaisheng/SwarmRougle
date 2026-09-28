// SRGameStateBase.cpp

#include "SRGameStateBase.h"
#include "Net/UnrealNetwork.h"

ASRGameStateBase::ASRGameStateBase()
{
    CurrentWave = 0;
}

void ASRGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // 将 CurrentWave 同步给所有客户端
    DOREPLIFETIME(ASRGameStateBase, CurrentWave);
}

void ASRGameStateBase::SetCurrentWave(int32 NewWave)
{
    // 仅限服务器调用
    if (HasAuthority())
    {
        // 记录旧值
        int32 OldWave = CurrentWave;
        CurrentWave = NewWave;
        // 服务器自己是不会触发 OnRep 的，如果是 Listen Server，需要手动调用或广播
        OnRep_CurrentWave(OldWave);
    }
}

void ASRGameStateBase::OnRep_CurrentWave(int32 OldWave)
{
    // 客户端收到波次更新后，触发委托
    // WBP_HUD 只需要在 Construct 时绑定这个委托即可，实现 UI 与逻辑彻底解耦
    if (CurrentWave != OldWave)
    {
        // 客户端收到波次更新后，触发委托，解耦 UI
        OnWaveChangedDelegate.Broadcast(CurrentWave);
    }
}

void ASRGameStateBase::RegisterPlayer(APawn* Player)
{
    // 只有服务器有资格修改生死簿
    if (HasAuthority() && IsValid(Player) && !ActivePlayers.Contains(Player))
    {
        ActivePlayers.Add(Player);
        UE_LOG(LogTemp, Log, TEXT("玩家已注册进 GameState 的活跃列表。当前人数: %d"), ActivePlayers.Num());
    }
}

void ASRGameStateBase::UnregisterPlayer(APawn* Player)
{
    if (HasAuthority() && Player)
    {
        ActivePlayers.Remove(Player);
        UE_LOG(LogTemp, Log, TEXT("玩家已从 GameState 注销。当前人数: %d"), ActivePlayers.Num());
    }
}