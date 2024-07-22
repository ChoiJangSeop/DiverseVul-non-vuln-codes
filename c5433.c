static Image *ReadSFWImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  static unsigned char
    HuffmanTable[] =
    {
      0xFF, 0xC4, 0x01, 0xA2, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01, 0x01,
      0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
      0x01, 0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
      0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04,
      0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x10, 0x00, 0x02, 0x01,
      0x03, 0x03, 0x02, 0x04, 0x03, 0x05, 0x05, 0x04, 0x04, 0x00, 0x00,
      0x01, 0x7D, 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21,
      0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32,
      0x81, 0x91, 0xA1, 0x08, 0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1,
      0xF0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16, 0x17, 0x18,
      0x19, 0x1A, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x34, 0x35, 0x36,
      0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
      0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63, 0x64,
      0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77,
      0x78, 0x79, 0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A,
      0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3,
      0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5,
      0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
      0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9,
      0xDA, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA,
      0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0x11,
      0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04, 0x07, 0x05, 0x04,
      0x04, 0x00, 0x01, 0x02, 0x77, 0x00, 0x01, 0x02, 0x03, 0x11, 0x04,
      0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71, 0x13,
      0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xA1, 0xB1, 0xC1, 0x09,
      0x23, 0x33, 0x52, 0xF0, 0x15, 0x62, 0x72, 0xD1, 0x0A, 0x16, 0x24,
      0x34, 0xE1, 0x25, 0xF1, 0x17, 0x18, 0x19, 0x1A, 0x26, 0x27, 0x28,
      0x29, 0x2A, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45,
      0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
      0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73,
      0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x82, 0x83, 0x84, 0x85,
      0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
      0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9,
      0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2,
      0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4,
      0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6,
      0xE7, 0xE8, 0xE9, 0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
      0xF9, 0xFA
    };

  FILE
    *file;

  Image
    *flipped_image,
    *jpeg_image,
    *image;

  ImageInfo
    *read_info;

  int
    unique_file;

  MagickBooleanType
    status;

  register unsigned char
    *header,
    *data;

  size_t
    extent;

  ssize_t
    count;

  unsigned char
    *buffer,
    *offset;

  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  image=AcquireImage(image_info,exception);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  /*
    Read image into a buffer.
  */
  if (GetBlobSize(image) != (size_t) GetBlobSize(image))
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
  buffer=(unsigned char *) AcquireQuantumMemory((size_t) GetBlobSize(image),
    sizeof(*buffer));
  if (buffer == (unsigned char *) NULL)
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
  count=ReadBlob(image,(size_t) GetBlobSize(image),buffer);
  if ((count != (ssize_t) GetBlobSize(image)) ||
      (LocaleNCompare((char *) buffer,"SFW",3) != 0))
    {
      buffer=(unsigned char *) RelinquishMagickMemory(buffer);
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    }
  (void) CloseBlob(image);
  /*
    Find the start of the JFIF data
  */
  header=SFWScan(buffer,buffer+count-1,(const unsigned char *)
    "\377\310\377\320",4);
  if (header == (unsigned char *) NULL)
    {
      buffer=(unsigned char *) RelinquishMagickMemory(buffer);
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    }
  TranslateSFWMarker(header);  /* translate soi and app tags */
  TranslateSFWMarker(header+2);
  (void) CopyMagickMemory(header+6,"JFIF\0\001\0",7);  /* JFIF magic */
  /*
    Translate remaining markers.
  */
  offset=header+2;
  offset+=(((unsigned int) offset[2]) << 8)+offset[3]+2;
  for ( ; ; )
  {
    if ((offset+4) > (buffer+count-1))
      {
        buffer=(unsigned char *) RelinquishMagickMemory(buffer);
        ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
      }
    TranslateSFWMarker(offset);
    if (offset[1] == 0xda)
      break;
    offset+=(((unsigned int) offset[2]) << 8)+offset[3]+2;
  }
  offset--;
  data=SFWScan(offset,buffer+count-1,(const unsigned char *) "\377\311",2);
  if (data == (unsigned char *) NULL)
    {
      buffer=(unsigned char *) RelinquishMagickMemory(buffer);
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    }
  TranslateSFWMarker(data++);  /* translate eoi marker */
  /*
    Write JFIF file.
  */
  read_info=CloneImageInfo(image_info);
  SetImageInfoBlob(read_info,(void *) NULL,0);
  file=(FILE *) NULL;
  unique_file=AcquireUniqueFileResource(read_info->filename);
  if (unique_file != -1)
    file=fopen_utf8(read_info->filename,"wb");
  if ((unique_file == -1) || (file == (FILE *) NULL))
    {
      buffer=(unsigned char *) RelinquishMagickMemory(buffer);
      read_info=DestroyImageInfo(read_info);
      (void) CopyMagickString(image->filename,read_info->filename,
        MagickPathExtent);
      ThrowFileException(exception,FileOpenError,"UnableToCreateTemporaryFile",
        image->filename);
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  extent=fwrite(header,(size_t) (offset-header+1),1,file);
  (void) extent;
  extent=fwrite(HuffmanTable,1,sizeof(HuffmanTable)/sizeof(*HuffmanTable),file);
  extent=fwrite(offset+1,(size_t) (data-offset),1,file);
  status=ferror(file) != 0 ? MagickFalse : MagickTrue;
  (void) fclose(file);
  (void) close(unique_file);
  buffer=(unsigned char *) RelinquishMagickMemory(buffer);
  if (status == MagickFalse)
    {
      char
        *message;

      (void) remove_utf8(read_info->filename);
      read_info=DestroyImageInfo(read_info);
      message=GetExceptionMessage(errno);
      (void) ThrowMagickException(exception,GetMagickModule(),FileOpenError,
        "UnableToWriteFile","`%s': %s",image->filename,message);
      message=DestroyString(message);
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  /*
    Read JPEG image.
  */
  jpeg_image=ReadImage(read_info,exception);
  (void) RelinquishUniqueFileResource(read_info->filename);
  read_info=DestroyImageInfo(read_info);
  if (jpeg_image == (Image *) NULL)
    {
      image=DestroyImageList(image);
      return(jpeg_image);
    }
  (void) CopyMagickString(jpeg_image->filename,image->filename,MagickPathExtent);
  (void) CopyMagickString(jpeg_image->magick,image->magick,MagickPathExtent);
  image=DestroyImageList(image);
  image=jpeg_image;
  /*
    Correct image orientation.
  */
  flipped_image=FlipImage(image,exception);
  if (flipped_image != (Image *) NULL)
    {
      DuplicateBlob(flipped_image,image);
      image=DestroyImage(image);
      image=flipped_image;
    }
  return(GetFirstImageInList(image));
}