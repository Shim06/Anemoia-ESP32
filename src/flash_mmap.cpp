#include "flash_mmap.h"
#include "core/cartridge.h"

struct PartitionHeader
{
    uint32_t magic;
    uint32_t crc32;
};
static constexpr uint32_t PART_MAGIC = 0x4E455321u; // 'NES!'

bool mappedROM_init(MappedROM* rom, Cartridge* cart, uint32_t crc32, uint8_t num_prg_banks_16k,
                    uint8_t num_chr_banks_8k)
{
    const esp_partition_t* ptr_partition =
        esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "nesrom");
    if (!ptr_partition)
    {
        LOG("[flash_mmap] 'nesrom' partition not found.");
        return false;
    }

    const uint32_t prg_size = (uint32_t)num_prg_banks_16k * (16U * 1024U);
    const uint32_t chr_size = (uint32_t)num_chr_banks_8k * (8U * 1024U);
    const uint32_t body_size = prg_size + chr_size;

    if (sizeof(PartitionHeader) + body_size > ptr_partition->size)
    {
        LOGF("[flash_mmap] ROM (%u B) exceeds partition (%u B)\n", (unsigned)body_size,
             (unsigned)ptr_partition->size);
        return false;
    }

    // Write only if ROM is different from previously loaded one (based on CRC32)
    PartitionHeader stored = {};
    esp_partition_read(ptr_partition, 0, &stored, sizeof(stored));
    if (stored.magic != PART_MAGIC || stored.crc32 != crc32)
    {
        LOG("[flash_mmap] Writing ROM to flash...");

        if (esp_partition_erase_range(ptr_partition, 0, ptr_partition->size) != ESP_OK)
        {
            LOG("[flash_mmap] Erase failed.");
            return false;
        }

        {
            uint8_t buf[4096];
            PartitionHeader header = { PART_MAGIC, crc32 };
            esp_partition_write(ptr_partition, 0, &header, sizeof(header));

            cart->seek(16); // skip iNES header
            for (uint32_t offset = sizeof(PartitionHeader),
                          remaining = body_size + sizeof(PartitionHeader);
                 offset < remaining; offset += sizeof(buf))
            {
                cart->read(buf, sizeof(buf));
                esp_partition_write(ptr_partition, offset, buf, sizeof(buf));
            }
        }
        LOG("[flash_mmap] Done.");
    }
    else LOG("[flash_mmap] ROM already in flash, skipping write.");

    const void* mapped;
    esp_err_t err = esp_partition_mmap(ptr_partition, 0, body_size + sizeof(PartitionHeader),
                                       ESP_PARTITION_MMAP_DATA, &mapped, &rom->mmap_handle);
    if (err != ESP_OK)
    {
        LOGF("[flash_mmap] mmap failed: %s\n", esp_err_to_name(err));
        return false;
    }

    rom->prg_base = ((const uint8_t*)mapped) + sizeof(PartitionHeader);
    rom->chr_base = rom->prg_base + prg_size;
    rom->prg_size = prg_size;
    rom->chr_size = chr_size;
    LOGF("[flash_mmap] mmap result: %s, prg_base: %p\n", esp_err_to_name(err), rom->prg_base);
    return true;
}