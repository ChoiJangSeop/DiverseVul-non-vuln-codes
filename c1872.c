static int vdi_create(const char *filename, QEMUOptionParameter *options,
                      Error **errp)
{
    int fd;
    int result = 0;
    uint64_t bytes = 0;
    uint32_t blocks;
    size_t block_size = DEFAULT_CLUSTER_SIZE;
    uint32_t image_type = VDI_TYPE_DYNAMIC;
    VdiHeader header;
    size_t i;
    size_t bmap_size;

    logout("\n");

    /* Read out options. */
    while (options && options->name) {
        if (!strcmp(options->name, BLOCK_OPT_SIZE)) {
            bytes = options->value.n;
#if defined(CONFIG_VDI_BLOCK_SIZE)
        } else if (!strcmp(options->name, BLOCK_OPT_CLUSTER_SIZE)) {
            if (options->value.n) {
                /* TODO: Additional checks (SECTOR_SIZE * 2^n, ...). */
                block_size = options->value.n;
            }
#endif
#if defined(CONFIG_VDI_STATIC_IMAGE)
        } else if (!strcmp(options->name, BLOCK_OPT_STATIC)) {
            if (options->value.n) {
                image_type = VDI_TYPE_STATIC;
            }
#endif
        }
        options++;
    }

    fd = qemu_open(filename,
                   O_WRONLY | O_CREAT | O_TRUNC | O_BINARY | O_LARGEFILE,
                   0644);
    if (fd < 0) {
        return -errno;
    }

    /* We need enough blocks to store the given disk size,
       so always round up. */
    blocks = (bytes + block_size - 1) / block_size;

    bmap_size = blocks * sizeof(uint32_t);
    bmap_size = ((bmap_size + SECTOR_SIZE - 1) & ~(SECTOR_SIZE -1));

    memset(&header, 0, sizeof(header));
    pstrcpy(header.text, sizeof(header.text), VDI_TEXT);
    header.signature = VDI_SIGNATURE;
    header.version = VDI_VERSION_1_1;
    header.header_size = 0x180;
    header.image_type = image_type;
    header.offset_bmap = 0x200;
    header.offset_data = 0x200 + bmap_size;
    header.sector_size = SECTOR_SIZE;
    header.disk_size = bytes;
    header.block_size = block_size;
    header.blocks_in_image = blocks;
    if (image_type == VDI_TYPE_STATIC) {
        header.blocks_allocated = blocks;
    }
    uuid_generate(header.uuid_image);
    uuid_generate(header.uuid_last_snap);
    /* There is no need to set header.uuid_link or header.uuid_parent here. */
#if defined(CONFIG_VDI_DEBUG)
    vdi_header_print(&header);
#endif
    vdi_header_to_le(&header);
    if (write(fd, &header, sizeof(header)) < 0) {
        result = -errno;
    }

    if (bmap_size > 0) {
        uint32_t *bmap = g_malloc0(bmap_size);
        for (i = 0; i < blocks; i++) {
            if (image_type == VDI_TYPE_STATIC) {
                bmap[i] = i;
            } else {
                bmap[i] = VDI_UNALLOCATED;
            }
        }
        if (write(fd, bmap, bmap_size) < 0) {
            result = -errno;
        }
        g_free(bmap);
    }

    if (image_type == VDI_TYPE_STATIC) {
        if (ftruncate(fd, sizeof(header) + bmap_size + blocks * block_size)) {
            result = -errno;
        }
    }

    if (close(fd) < 0) {
        result = -errno;
    }

    return result;
}