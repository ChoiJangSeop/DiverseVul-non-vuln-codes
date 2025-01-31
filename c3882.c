MagickExport MagickBooleanType DrawPrimitive(Image *image,
  const DrawInfo *draw_info,const PrimitiveInfo *primitive_info,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;

  MagickStatusType
    status;

  register ssize_t
    i,
    x;

  ssize_t
    y;

  if (image->debug != MagickFalse)
    {
      (void) LogMagickEvent(DrawEvent,GetMagickModule(),
        "  begin draw-primitive");
      (void) LogMagickEvent(DrawEvent,GetMagickModule(),
        "    affine: %g %g %g %g %g %g",draw_info->affine.sx,
        draw_info->affine.rx,draw_info->affine.ry,draw_info->affine.sy,
        draw_info->affine.tx,draw_info->affine.ty);
    }
  if ((IsGrayColorspace(image->colorspace) != MagickFalse) &&
      ((IsPixelInfoGray(&draw_info->fill) == MagickFalse) ||
       (IsPixelInfoGray(&draw_info->stroke) == MagickFalse)))
    (void) SetImageColorspace(image,sRGBColorspace,exception);
  status=MagickTrue;
  x=(ssize_t) ceil(primitive_info->point.x-0.5);
  y=(ssize_t) ceil(primitive_info->point.y-0.5);
  image_view=AcquireAuthenticCacheView(image,exception);
  switch (primitive_info->primitive)
  {
    case AlphaPrimitive:
    {
      if (image->alpha_trait == UndefinedPixelTrait)
        (void) SetImageAlphaChannel(image,OpaqueAlphaChannel,exception);
      switch (primitive_info->method)
      {
        case PointMethod:
        default:
        {
          PixelInfo
            pixel;

          register Quantum
            *q;

          q=GetCacheViewAuthenticPixels(image_view,x,y,1,1,exception);
          if (q == (Quantum *) NULL)
            break;
          GetFillColor(draw_info,x,y,&pixel,exception);
          SetPixelAlpha(image,ClampToQuantum(pixel.alpha),q);
          (void) SyncCacheViewAuthenticPixels(image_view,exception);
          break;
        }
        case ReplaceMethod:
        {
          MagickBooleanType
            sync;

          PixelInfo
            pixel,
            target;

          (void) GetOneCacheViewVirtualPixelInfo(image_view,x,y,&target,
            exception);
          GetPixelInfo(image,&pixel);
          for (y=0; y < (ssize_t) image->rows; y++)
          {
            register Quantum
              *magick_restrict q;

            q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,
              exception);
            if (q == (Quantum *) NULL)
              break;
            for (x=0; x < (ssize_t) image->columns; x++)
            {
              GetPixelInfoPixel(image,q,&pixel);
              if (IsFuzzyEquivalencePixelInfo(&pixel,&target) == MagickFalse)
                {
                  q+=GetPixelChannels(image);
                  continue;
                }
              GetFillColor(draw_info,x,y,&pixel,exception);
              SetPixelAlpha(image,ClampToQuantum(pixel.alpha),q);
              q+=GetPixelChannels(image);
            }
            sync=SyncCacheViewAuthenticPixels(image_view,exception);
            if (sync == MagickFalse)
              break;
          }
          break;
        }
        case FloodfillMethod:
        case FillToBorderMethod:
        {
          ChannelType
            channel_mask;

          PixelInfo
            target;

          (void) GetOneVirtualPixelInfo(image,TileVirtualPixelMethod,x,y,
            &target,exception);
          if (primitive_info->method == FillToBorderMethod)
            {
              target.red=(double) draw_info->border_color.red;
              target.green=(double) draw_info->border_color.green;
              target.blue=(double) draw_info->border_color.blue;
            }
          channel_mask=SetImageChannelMask(image,AlphaChannel);
          status&=FloodfillPaintImage(image,draw_info,&target,x,y,
            primitive_info->method == FloodfillMethod ? MagickFalse :
            MagickTrue,exception);
          (void) SetImageChannelMask(image,channel_mask);
          break;
        }
        case ResetMethod:
        {
          MagickBooleanType
            sync;

          PixelInfo
            pixel;

          for (y=0; y < (ssize_t) image->rows; y++)
          {
            register Quantum
              *magick_restrict q;

            q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,
              exception);
            if (q == (Quantum *) NULL)
              break;
            for (x=0; x < (ssize_t) image->columns; x++)
            {
              GetFillColor(draw_info,x,y,&pixel,exception);
              SetPixelAlpha(image,ClampToQuantum(pixel.alpha),q);
              q+=GetPixelChannels(image);
            }
            sync=SyncCacheViewAuthenticPixels(image_view,exception);
            if (sync == MagickFalse)
              break;
          }
          break;
        }
      }
      break;
    }
    case ColorPrimitive:
    {
      switch (primitive_info->method)
      {
        case PointMethod:
        default:
        {
          PixelInfo
            pixel;

          register Quantum
            *q;

          q=GetCacheViewAuthenticPixels(image_view,x,y,1,1,exception);
          if (q == (Quantum *) NULL)
            break;
          GetPixelInfo(image,&pixel);
          GetFillColor(draw_info,x,y,&pixel,exception);
          SetPixelViaPixelInfo(image,&pixel,q);
          (void) SyncCacheViewAuthenticPixels(image_view,exception);
          break;
        }
        case ReplaceMethod:
        {
          MagickBooleanType
            sync;

          PixelInfo
            pixel,
            target;

          (void) GetOneCacheViewVirtualPixelInfo(image_view,x,y,&target,
            exception);
          for (y=0; y < (ssize_t) image->rows; y++)
          {
            register Quantum
              *magick_restrict q;

            q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,
              exception);
            if (q == (Quantum *) NULL)
              break;
            for (x=0; x < (ssize_t) image->columns; x++)
            {
              GetPixelInfoPixel(image,q,&pixel);
              if (IsFuzzyEquivalencePixelInfo(&pixel,&target) == MagickFalse)
                {
                  q+=GetPixelChannels(image);
                  continue;
                }
              GetFillColor(draw_info,x,y,&pixel,exception);
              SetPixelViaPixelInfo(image,&pixel,q);
              q+=GetPixelChannels(image);
            }
            sync=SyncCacheViewAuthenticPixels(image_view,exception);
            if (sync == MagickFalse)
              break;
          }
          break;
        }
        case FloodfillMethod:
        case FillToBorderMethod:
        {
          PixelInfo
            target;

          (void) GetOneVirtualPixelInfo(image,TileVirtualPixelMethod,x,y,
            &target,exception);
          if (primitive_info->method == FillToBorderMethod)
            {
              target.red=(double) draw_info->border_color.red;
              target.green=(double) draw_info->border_color.green;
              target.blue=(double) draw_info->border_color.blue;
            }
          status&=FloodfillPaintImage(image,draw_info,&target,x,y,
            primitive_info->method == FloodfillMethod ? MagickFalse :
            MagickTrue,exception);
          break;
        }
        case ResetMethod:
        {
          MagickBooleanType
            sync;

          PixelInfo
            pixel;

          GetPixelInfo(image,&pixel);
          for (y=0; y < (ssize_t) image->rows; y++)
          {
            register Quantum
              *magick_restrict q;

            q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,
              exception);
            if (q == (Quantum *) NULL)
              break;
            for (x=0; x < (ssize_t) image->columns; x++)
            {
              GetFillColor(draw_info,x,y,&pixel,exception);
              SetPixelViaPixelInfo(image,&pixel,q);
              q+=GetPixelChannels(image);
            }
            sync=SyncCacheViewAuthenticPixels(image_view,exception);
            if (sync == MagickFalse)
              break;
          }
          break;
        }
      }
      break;
    }
    case ImagePrimitive:
    {
      AffineMatrix
        affine;

      char
        composite_geometry[MagickPathExtent];

      Image
        *composite_image;

      ImageInfo
        *clone_info;

      RectangleInfo
        geometry;

      ssize_t
        x1,
        y1;

      if (primitive_info->text == (char *) NULL)
        break;
      clone_info=AcquireImageInfo();
      if (LocaleNCompare(primitive_info->text,"data:",5) == 0)
        composite_image=ReadInlineImage(clone_info,primitive_info->text,
          exception);
      else
        {
          (void) CopyMagickString(clone_info->filename,primitive_info->text,
            MagickPathExtent);
          composite_image=ReadImage(clone_info,exception);
        }
      clone_info=DestroyImageInfo(clone_info);
      if (composite_image == (Image *) NULL)
        break;
      (void) SetImageProgressMonitor(composite_image,(MagickProgressMonitor)
        NULL,(void *) NULL);
      x1=(ssize_t) ceil(primitive_info[1].point.x-0.5);
      y1=(ssize_t) ceil(primitive_info[1].point.y-0.5);
      if (((x1 != 0L) && (x1 != (ssize_t) composite_image->columns)) ||
          ((y1 != 0L) && (y1 != (ssize_t) composite_image->rows)))
        {
          /*
            Resize image.
          */
          (void) FormatLocaleString(composite_geometry,MagickPathExtent,
            "%gx%g!",primitive_info[1].point.x,primitive_info[1].point.y);
          composite_image->filter=image->filter;
          (void) TransformImage(&composite_image,(char *) NULL,
            composite_geometry,exception);
        }
      if (composite_image->alpha_trait == UndefinedPixelTrait)
        (void) SetImageAlphaChannel(composite_image,OpaqueAlphaChannel,
          exception);
      if (draw_info->alpha != OpaqueAlpha)
        (void) SetImageAlpha(composite_image,draw_info->alpha,exception);
      SetGeometry(image,&geometry);
      image->gravity=draw_info->gravity;
      geometry.x=x;
      geometry.y=y;
      (void) FormatLocaleString(composite_geometry,MagickPathExtent,
        "%.20gx%.20g%+.20g%+.20g",(double) composite_image->columns,(double)
        composite_image->rows,(double) geometry.x,(double) geometry.y);
      (void) ParseGravityGeometry(image,composite_geometry,&geometry,exception);
      affine=draw_info->affine;
      affine.tx=(double) geometry.x;
      affine.ty=(double) geometry.y;
      composite_image->interpolate=image->interpolate;
      if (draw_info->compose == OverCompositeOp)
        (void) DrawAffineImage(image,composite_image,&affine,exception);
      else
        (void) CompositeImage(image,composite_image,draw_info->compose,
          MagickTrue,geometry.x,geometry.y,exception);
      composite_image=DestroyImage(composite_image);
      break;
    }
    case PointPrimitive:
    {
      PixelInfo
        fill_color;

      register Quantum
        *q;

      if ((y < 0) || (y >= (ssize_t) image->rows))
        break;
      if ((x < 0) || (x >= (ssize_t) image->columns))
        break;
      q=GetCacheViewAuthenticPixels(image_view,x,y,1,1,exception);
      if (q == (Quantum *) NULL)
        break;
      GetFillColor(draw_info,x,y,&fill_color,exception);
      CompositePixelOver(image,&fill_color,(double) fill_color.alpha,q,
        (double) GetPixelAlpha(image,q),q);
      (void) SyncCacheViewAuthenticPixels(image_view,exception);
      break;
    }
    case TextPrimitive:
    {
      char
        geometry[MagickPathExtent];

      DrawInfo
        *clone_info;

      if (primitive_info->text == (char *) NULL)
        break;
      clone_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
      (void) CloneString(&clone_info->text,primitive_info->text);
      (void) FormatLocaleString(geometry,MagickPathExtent,"%+f%+f",
        primitive_info->point.x,primitive_info->point.y);
      (void) CloneString(&clone_info->geometry,geometry);
      status&=AnnotateImage(image,clone_info,exception);
      clone_info=DestroyDrawInfo(clone_info);
      break;
    }
    default:
    {
      double
        mid,
        scale;

      DrawInfo
        *clone_info;

      if (IsEventLogging() != MagickFalse)
        LogPrimitiveInfo(primitive_info);
      scale=ExpandAffine(&draw_info->affine);
      if ((draw_info->dash_pattern != (double *) NULL) &&
          (draw_info->dash_pattern[0] != 0.0) &&
          ((scale*draw_info->stroke_width) >= MagickEpsilon) &&
          (draw_info->stroke.alpha != (Quantum) TransparentAlpha))
        {
          /*
            Draw dash polygon.
          */
          clone_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
          clone_info->stroke_width=0.0;
          clone_info->stroke.alpha=(Quantum) TransparentAlpha;
          status&=DrawPolygonPrimitive(image,clone_info,primitive_info,
            exception);
          clone_info=DestroyDrawInfo(clone_info);
          (void) DrawDashPolygon(draw_info,primitive_info,image,exception);
          break;
        }
      mid=ExpandAffine(&draw_info->affine)*draw_info->stroke_width/2.0;
      if ((mid > 1.0) &&
          ((draw_info->stroke.alpha != (Quantum) TransparentAlpha) ||
           (draw_info->stroke_pattern != (Image *) NULL)))
        {
          MagickBooleanType
            closed_path;

          /*
            Draw strokes while respecting line cap/join attributes.
          */
          for (i=0; primitive_info[i].primitive != UndefinedPrimitive; i++) ;
          closed_path=
            (primitive_info[i-1].point.x == primitive_info[0].point.x) &&
            (primitive_info[i-1].point.y == primitive_info[0].point.y) ?
            MagickTrue : MagickFalse;
          i=(ssize_t) primitive_info[0].coordinates;
          if ((((draw_info->linecap == RoundCap) ||
                (closed_path != MagickFalse)) &&
               (draw_info->linejoin == RoundJoin)) ||
               (primitive_info[i].primitive != UndefinedPrimitive))
            {
              (void) DrawPolygonPrimitive(image,draw_info,primitive_info,
                exception);
              break;
            }
          clone_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
          clone_info->stroke_width=0.0;
          clone_info->stroke.alpha=(Quantum) TransparentAlpha;
          status&=DrawPolygonPrimitive(image,clone_info,primitive_info,
            exception);
          clone_info=DestroyDrawInfo(clone_info);
          status&=DrawStrokePolygon(image,draw_info,primitive_info,exception);
          break;
        }
      status&=DrawPolygonPrimitive(image,draw_info,primitive_info,exception);
      break;
    }
  }
  image_view=DestroyCacheView(image_view);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),"  end draw-primitive");
  return(status != 0 ? MagickTrue : MagickFalse);
}