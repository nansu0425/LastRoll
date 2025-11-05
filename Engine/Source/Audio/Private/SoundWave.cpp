#include "pch.h"

#include "Audio/Public/SoundWave.h"

#define FOURCC_RIFF 'FFIR'
#define FOURCC_WAVE 'EVAW'
#define FOURCC_FMT  ' tmf'
#define FOURCC_DATA 'atad'

IMPLEMENT_CLASS(USoundWave, UObject)

USoundWave::USoundWave()
    : WaveData(nullptr), WaveDataSize(0)
{
    ZeroMemory(&WaveFormat, sizeof(WaveFormat));
}

USoundWave::~USoundWave()
{
    ClearData();
}

bool USoundWave::LoadFromFile(const FString& InFileName)
{
    ClearData();

    std::ifstream File(InFileName, std::ios::binary);
    if (!File.is_open())
    {
        UE_LOG_ERROR("파일을 여는데 실패했습니다: %s", InFileName.c_str());
        return false;
    }

    DWORD ChunkID = 0;
    DWORD FileSize = 0;
    File.read(reinterpret_cast<char*>(&ChunkID), sizeof(DWORD));
    File.read(reinterpret_cast<char*>(&FileSize), sizeof(DWORD));
    if (ChunkID != FOURCC_RIFF)
    {
        File.close();
        return false;
    }

    DWORD Format = 0;
    File.read(reinterpret_cast<char*>(&Format), sizeof(DWORD));
    if (Format != FOURCC_WAVE)
    {
        File.close();
        return false;
    }

    DWORD FmtChunkSize = 0;
    DWORD FmtChunkPos = 0;
    if (!FindChunk(File, FOURCC_FMT, FmtChunkSize, FmtChunkPos))
    {
        File.close();
        return false;
    }
    if (!ReadChunkData(File, &WaveFormat, FmtChunkSize, FmtChunkPos))
    {
        File.close();
        return false;
    }
    if (WaveFormat.wFormatTag != WAVE_FORMAT_PCM)
    {
        File.close();
        return false;
    }
    if (!FindChunk(File, FOURCC_DATA, WaveDataSize, FmtChunkPos))
    {
        File.close();
        return false;
    }

    WaveData = new BYTE[WaveDataSize];
    if (!WaveData)
    {
        File.close();
        return false;
    }
    if (!ReadChunkData(File, WaveData, WaveDataSize, FmtChunkPos))
    {
        delete[] WaveData;
        WaveData = nullptr;
        File.close();
        return false;
    }
    File.close();
    return true;
}

bool USoundWave::FindChunk(std::ifstream& InFile, DWORD FourCC, DWORD& OutChunkSize, DWORD& OutChunkPos)
{
    // RIFF 헤더 이후부터 탐색 시작
    InFile.seekg(12, std::ios::beg);

    DWORD ChunkID = 0;
    DWORD ChunkSize = 0;

    while (InFile.good())
    {
        if (!InFile.read(reinterpret_cast<char*>(&ChunkID), sizeof(DWORD)))
        {
            break; 
        }

        if (!InFile.read(reinterpret_cast<char*>(&ChunkSize), sizeof(DWORD)))
        {
            break;
        }

        if (ChunkID == FourCC)
        {
            OutChunkSize = ChunkSize;
            OutChunkPos = static_cast<DWORD>(InFile.tellg());
            return true;
        }

        InFile.seekg(ChunkSize, std::ios::cur);
    }

    return false;
}

bool USoundWave::ReadChunkData(std::ifstream& InFile, void* InBuffer, DWORD InBufferSize, DWORD InBufferPos)
{
    InFile.seekg(InBufferPos, std::ios::beg);
    if (!InFile.read(reinterpret_cast<char*>(InBuffer), InBufferSize))
    {
        return false;
    }
    return true;
}

void USoundWave::ClearData()
{
    if (WaveData)
    {
        delete[] WaveData;
        WaveData = nullptr;
    }
    WaveDataSize = 0;
    ZeroMemory(&WaveFormat, sizeof(WaveFormat));
}