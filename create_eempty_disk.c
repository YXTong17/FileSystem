FILE* f=fopen("disk","w");
    char a[DISK_BLK/1024][BLOCK_SIZE];
    for(int i=0;i<1024;i++)
        fwrite(a,BLOCK_SIZE,DISK_BLK/1024,f);
    fclose(f);
