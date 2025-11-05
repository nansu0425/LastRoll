#pragma once

UCLASS()
class USoundWave : public UObject
{
    GENERATED_BODY()
    DECLARE_CLASS(USoundWave, UObject)
public:
    USoundWave();

    virtual ~USoundWave();

    bool LoadFromFile(const FString& InFileName);

public:
    /** XAudio2 포맷 정보 */
    WAVEFORMATEX WaveFormat;

    /** 오디오 데이터 저장용 버퍼 */
    BYTE* WaveData;

    /** 오디오 데이터 크기 (바이트 단위) */
    DWORD WaveDataSize;
    
private:
    bool FindChunk(std::ifstream& InFile, DWORD FourCC, DWORD& OutChunkSize, DWORD& OutChunkPos);
    
    bool ReadChunkData(std::ifstream& InFile, void* InBuffer, DWORD InBufferSize, DWORD InBufferPos);

    void ClearData();
};