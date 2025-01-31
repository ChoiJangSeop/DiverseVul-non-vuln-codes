MagickExport MagickBooleanType DrawImage(Image *image,const DrawInfo *draw_info,
  ExceptionInfo *exception)
{
#define RenderImageTag  "Render/Image"

  AffineMatrix
    affine,
    current;

  char
    keyword[MagickPathExtent],
    geometry[MagickPathExtent],
    pattern[MagickPathExtent],
    *primitive,
    *token;

  const char
    *q;

  DrawInfo
    **graphic_context;

  MagickBooleanType
    proceed;

  MagickStatusType
    status;

  double
    angle,
    factor,
    primitive_extent;

  PointInfo
    point;

  PrimitiveInfo
    *primitive_info;

  PrimitiveType
    primitive_type;

  register const char
    *p;

  register ssize_t
    i,
    x;

  SegmentInfo
    bounds;

  size_t
    extent,
    length,
    number_points,
    number_stops;

  ssize_t
    j,
    k,
    n;

  StopInfo
    *stops;

  /*
    Ensure the annotation info is valid.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(draw_info != (DrawInfo *) NULL);
  assert(draw_info->signature == MagickCoreSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  if ((draw_info->primitive == (char *) NULL) ||
      (*draw_info->primitive == '\0'))
    return(MagickFalse);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),"begin draw-image");
  if (*draw_info->primitive != '@')
    primitive=AcquireString(draw_info->primitive);
  else
    primitive=FileToString(draw_info->primitive+1,~0UL,exception);
  if (primitive == (char *) NULL)
    return(MagickFalse);
  primitive_extent=(double) strlen(primitive);
  (void) SetImageArtifact(image,"MVG",primitive);
  n=0;
  number_stops=0;
  stops=(StopInfo *) NULL;
  /*
    Allocate primitive info memory.
  */
  graphic_context=(DrawInfo **) AcquireMagickMemory(
    sizeof(*graphic_context));
  if (graphic_context == (DrawInfo **) NULL)
    {
      primitive=DestroyString(primitive);
      ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
        image->filename);
    }
  number_points=6553;
  primitive_info=(PrimitiveInfo *) AcquireQuantumMemory((size_t) number_points,
    sizeof(*primitive_info));
  if (primitive_info == (PrimitiveInfo *) NULL)
    {
      primitive=DestroyString(primitive);
      for ( ; n >= 0; n--)
        graphic_context[n]=DestroyDrawInfo(graphic_context[n]);
      graphic_context=(DrawInfo **) RelinquishMagickMemory(graphic_context);
      ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
        image->filename);
    }
  graphic_context[n]=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  graphic_context[n]->viewbox=image->page;
  if ((image->page.width == 0) || (image->page.height == 0))
    {
      graphic_context[n]->viewbox.width=image->columns;
      graphic_context[n]->viewbox.height=image->rows;
    }
  token=AcquireString(primitive);
  extent=strlen(token)+MagickPathExtent;
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  status=MagickTrue;
  for (q=primitive; *q != '\0'; )
  {
    /*
      Interpret graphic primitive.
    */
    GetNextToken(q,&q,extent,keyword);
    if (*keyword == '\0')
      break;
    if (*keyword == '#')
      {
        /*
          Comment.
        */
        while ((*q != '\n') && (*q != '\0'))
          q++;
        continue;
      }
    p=q-strlen(keyword)-1;
    primitive_type=UndefinedPrimitive;
    current=graphic_context[n]->affine;
    GetAffineMatrix(&affine);
    switch (*keyword)
    {
      case ';':
        break;
      case 'a':
      case 'A':
      {
        if (LocaleCompare("affine",keyword) == 0)
          {
            GetNextToken(q,&q,extent,token);
            affine.sx=StringToDouble(token,(char **) NULL);
            GetNextToken(q,&q,extent,token);
            if (*token == ',')
              GetNextToken(q,&q,extent,token);
            affine.rx=StringToDouble(token,(char **) NULL);
            GetNextToken(q,&q,extent,token);
            if (*token == ',')
              GetNextToken(q,&q,extent,token);
            affine.ry=StringToDouble(token,(char **) NULL);
            GetNextToken(q,&q,extent,token);
            if (*token == ',')
              GetNextToken(q,&q,extent,token);
            affine.sy=StringToDouble(token,(char **) NULL);
            GetNextToken(q,&q,extent,token);
            if (*token == ',')
              GetNextToken(q,&q,extent,token);
            affine.tx=StringToDouble(token,(char **) NULL);
            GetNextToken(q,&q,extent,token);
            if (*token == ',')
              GetNextToken(q,&q,extent,token);
            affine.ty=StringToDouble(token,(char **) NULL);
            break;
          }
        if (LocaleCompare("alpha",keyword) == 0)
          {
            primitive_type=AlphaPrimitive;
            break;
          }
        if (LocaleCompare("arc",keyword) == 0)
          {
            primitive_type=ArcPrimitive;
            break;
          }
        status=MagickFalse;
        break;
      }
      case 'b':
      case 'B':
      {
        if (LocaleCompare("bezier",keyword) == 0)
          {
            primitive_type=BezierPrimitive;
            break;
          }
        if (LocaleCompare("border-color",keyword) == 0)
          {
            GetNextToken(q,&q,extent,token);
            (void) QueryColorCompliance(token,AllCompliance,
              &graphic_context[n]->border_color,exception);
            break;
          }
        status=MagickFalse;
        break;
      }
      case 'c':
      case 'C':
      {
        if (LocaleCompare("clip-path",keyword) == 0)
          {
            /*
              Create clip mask.
            */
            GetNextToken(q,&q,extent,token);
            (void) CloneString(&graphic_context[n]->clip_mask,token);
            (void) DrawClipPath(image,graphic_context[n],
              graphic_context[n]->clip_mask,exception);
            break;
          }
        if (LocaleCompare("clip-rule",keyword) == 0)
          {
            ssize_t
              fill_rule;

            GetNextToken(q,&q,extent,token);
            fill_rule=ParseCommandOption(MagickFillRuleOptions,MagickFalse,
              token);
            if (fill_rule == -1)
              status=MagickFalse;
            else
              graphic_context[n]->fill_rule=(FillRule) fill_rule;
            break;
          }
        if (LocaleCompare("clip-units",keyword) == 0)
          {
            ssize_t
              clip_units;

            GetNextToken(q,&q,extent,token);
            clip_units=ParseCommandOption(MagickClipPathOptions,MagickFalse,
              token);
            if (clip_units == -1)
              {
                status=MagickFalse;
                break;
              }
            graphic_context[n]->clip_units=(ClipPathUnits) clip_units;
            if (clip_units == ObjectBoundingBox)
              {
                GetAffineMatrix(&current);
                affine.sx=draw_info->bounds.x2;
                affine.sy=draw_info->bounds.y2;
                affine.tx=draw_info->bounds.x1;
                affine.ty=draw_info->bounds.y1;
                break;
              }
            break;
          }
        if (LocaleCompare("circle",keyword) == 0)
          {
            primitive_type=CirclePrimitive;
            break;
          }
        if (LocaleCompare("color",keyword) == 0)
          {
            primitive_type=ColorPrimitive;
            break;
          }
        status=MagickFalse;
        break;
      }
      case 'd':
      case 'D':
      {
        if (LocaleCompare("decorate",keyword) == 0)
          {
            ssize_t
              decorate;

            GetNextToken(q,&q,extent,token);
            decorate=ParseCommandOption(MagickDecorateOptions,MagickFalse,
              token);
            if (decorate == -1)
              status=MagickFalse;
            else
              graphic_context[n]->decorate=(DecorationType) decorate;
            break;
          }
        if (LocaleCompare("density",keyword) == 0)
          {
            GetNextToken(q,&q,extent,token);
            (void) CloneString(&graphic_context[n]->density,token);
            break;
          }
        if (LocaleCompare("direction",keyword) == 0)
          {
            ssize_t
              direction;

            GetNextToken(q,&q,extent,token);
            direction=ParseCommandOption(MagickDirectionOptions,MagickFalse,
              token);
            if (direction == -1)
              status=MagickFalse;
            else
              graphic_context[n]->direction=(DirectionType) direction;
            break;
          }
        status=MagickFalse;
        break;
      }
      case 'e':
      case 'E':
      {
        if (LocaleCompare("ellipse",keyword) == 0)
          {
            primitive_type=EllipsePrimitive;
            break;
          }
        if (LocaleCompare("encoding",keyword) == 0)
          {
            GetNextToken(q,&q,extent,token);
            (void) CloneString(&graphic_context[n]->encoding,token);
            break;
          }
        status=MagickFalse;
        break;
      }
      case 'f':
      case 'F':
      {
        if (LocaleCompare("fill",keyword) == 0)
          {
            GetNextToken(q,&q,extent,token);
            (void) FormatLocaleString(pattern,MagickPathExtent,"%s",token);
            if (GetImageArtifact(image,pattern) != (const char *) NULL)
              (void) DrawPatternPath(image,draw_info,token,
                &graphic_context[n]->fill_pattern,exception);
            else
              {
                status&=QueryColorCompliance(token,AllCompliance,
                  &graphic_context[n]->fill,exception);
                if (status == MagickFalse)
                  {
                    ImageInfo
                      *pattern_info;

                    pattern_info=AcquireImageInfo();
                    (void) CopyMagickString(pattern_info->filename,token,
                      MagickPathExtent);
                    graphic_context[n]->fill_pattern=ReadImage(pattern_info,
                      exception);
                    CatchException(exception);
                    pattern_info=DestroyImageInfo(pattern_info);
                  }
              }
            break;
          }
        if (LocaleCompare("fill-opacity",keyword) == 0)
          {
            GetNextToken(q,&q,extent,token);
            factor=strchr(token,'%') != (char *) NULL ? 0.01 : 1.0;
            graphic_context[n]->fill.alpha=(double) QuantumRange*
              factor*StringToDouble(token,(char **) NULL);
            break;
          }
        if (LocaleCompare("fill-rule",keyword) == 0)
          {
            ssize_t
              fill_rule;

            GetNextToken(q,&q,extent,token);
            fill_rule=ParseCommandOption(MagickFillRuleOptions,MagickFalse,
              token);
            if (fill_rule == -1)
              status=MagickFalse;
            else
              graphic_context[n]->fill_rule=(FillRule) fill_rule;
            break;
          }
        if (LocaleCompare("font",keyword) == 0)
          {
            GetNextToken(q,&q,extent,token);
            (void) CloneString(&graphic_context[n]->font,token);
            if (LocaleCompare("none",token) == 0)
              graphic_context[n]->font=(char *)
                RelinquishMagickMemory(graphic_context[n]->font);
            break;
          }
        if (LocaleCompare("font-family",keyword) == 0)
          {
            GetNextToken(q,&q,extent,token);
            (void) CloneString(&graphic_context[n]->family,token);
            break;
          }
        if (LocaleCompare("font-size",keyword) == 0)
          {
            GetNextToken(q,&q,extent,token);
            graphic_context[n]->pointsize=StringToDouble(token,(char **) NULL);
            break;
          }
        if (LocaleCompare("font-stretch",keyword) == 0)
          {
            ssize_t
              stretch;

            GetNextToken(q,&q,extent,token);
            stretch=ParseCommandOption(MagickStretchOptions,MagickFalse,token);
            if (stretch == -1)
              status=MagickFalse;
            else
              graphic_context[n]->stretch=(StretchType) stretch;
            break;
          }
        if (LocaleCompare("font-style",keyword) == 0)
          {
            ssize_t
              style;

            GetNextToken(q,&q,extent,token);
            style=ParseCommandOption(MagickStyleOptions,MagickFalse,token);
            if (style == -1)
              status=MagickFalse;
            else
              graphic_context[n]->style=(StyleType) style;
            break;
          }
        if (LocaleCompare("font-weight",keyword) == 0)
          {
            ssize_t
              weight;

            GetNextToken(q,&q,extent,token);
            weight=ParseCommandOption(MagickWeightOptions,MagickFalse,token);
            if (weight == -1)
              weight=StringToUnsignedLong(token);
            graphic_context[n]->weight=(size_t) weight;
            break;
          }
        status=MagickFalse;
        break;
      }
      case 'g':
      case 'G':
      {
        if (LocaleCompare("gradient-units",keyword) == 0)
          {
            GetNextToken(q,&q,extent,token);
            break;
          }
        if (LocaleCompare("gravity",keyword) == 0)
          {
            ssize_t
              gravity;

            GetNextToken(q,&q,extent,token);
            gravity=ParseCommandOption(MagickGravityOptions,MagickFalse,token);
            if (gravity == -1)
              status=MagickFalse;
            else
              graphic_context[n]->gravity=(GravityType) gravity;
            break;
          }
        status=MagickFalse;
        break;
      }
      case 'i':
      case 'I':
      {
        if (LocaleCompare("image",keyword) == 0)
          {
            ssize_t
              compose;

            primitive_type=ImagePrimitive;
            GetNextToken(q,&q,extent,token);
            compose=ParseCommandOption(MagickComposeOptions,MagickFalse,token);
            if (compose == -1)
              status=MagickFalse;
            else
              graphic_context[n]->compose=(CompositeOperator) compose;
            break;
          }
        if (LocaleCompare("interline-spacing",keyword) == 0)
          {
            GetNextToken(q,&q,extent,token);
            graphic_context[n]->interline_spacing=StringToDouble(token,
              (char **) NULL);
            break;
          }
        if (LocaleCompare("interword-spacing",keyword) == 0)
          {
            GetNextToken(q,&q,extent,token);
            graphic_context[n]->interword_spacing=StringToDouble(token,
              (char **) NULL);
            break;
          }
        status=MagickFalse;
        break;
      }
      case 'k':
      case 'K':
      {
        if (LocaleCompare("kerning",keyword) == 0)
          {
            GetNextToken(q,&q,extent,token);
            graphic_context[n]->kerning=StringToDouble(token,(char **) NULL);
            break;
          }
        status=MagickFalse;
        break;
      }
      case 'l':
      case 'L':
      {
        if (LocaleCompare("line",keyword) == 0)
          primitive_type=LinePrimitive;
        else
          status=MagickFalse;
        break;
      }
      case 'o':
      case 'O':
      {
        if (LocaleCompare("offset",keyword) == 0)
          {
            GetNextToken(q,&q,extent,token);
            break;
          }
        if (LocaleCompare("opacity",keyword) == 0)
          {
            GetNextToken(q,&q,extent,token);
            factor=strchr(token,'%') != (char *) NULL ? 0.01 : 1.0;
            graphic_context[n]->alpha=ClampToQuantum(QuantumRange*(1.0-((1.0-
              QuantumScale*graphic_context[n]->alpha)*factor*
              StringToDouble(token,(char **) NULL))));
            graphic_context[n]->fill.alpha=(double) graphic_context[n]->alpha;
            graphic_context[n]->stroke.alpha=(double) graphic_context[n]->alpha;
            break;
          }
        status=MagickFalse;
        break;
      }
      case 'p':
      case 'P':
      {
        if (LocaleCompare("path",keyword) == 0)
          {
            primitive_type=PathPrimitive;
            break;
          }
        if (LocaleCompare("point",keyword) == 0)
          {
            primitive_type=PointPrimitive;
            break;
          }
        if (LocaleCompare("polyline",keyword) == 0)
          {
            primitive_type=PolylinePrimitive;
            break;
          }
        if (LocaleCompare("polygon",keyword) == 0)
          {
            primitive_type=PolygonPrimitive;
            break;
          }
        if (LocaleCompare("pop",keyword) == 0)
          {
            GetNextToken(q,&q,extent,token);
            if (LocaleCompare("clip-path",token) == 0)
              break;
            if (LocaleCompare("defs",token) == 0)
              break;
            if (LocaleCompare("gradient",token) == 0)
              break;
            if (LocaleCompare("graphic-context",token) == 0)
              {
                if (n <= 0)
                  {
                    (void) ThrowMagickException(exception,GetMagickModule(),
                      DrawError,"UnbalancedGraphicContextPushPop","`%s'",token);
                    n=0;
                    break;
                  }
                if (graphic_context[n]->clip_mask != (char *) NULL)
                  if (LocaleCompare(graphic_context[n]->clip_mask,
                      graphic_context[n-1]->clip_mask) != 0)
                    (void) SetImageMask(image,ReadPixelMask,(Image *) NULL,
                      exception);
                graphic_context[n]=DestroyDrawInfo(graphic_context[n]);
                n--;
                break;
              }
            if (LocaleCompare("pattern",token) == 0)
              break;
            status=MagickFalse;
            break;
          }
        if (LocaleCompare("push",keyword) == 0)
          {
            GetNextToken(q,&q,extent,token);
            if (LocaleCompare("clip-path",token) == 0)
              {
                char
                  name[MagickPathExtent];

                GetNextToken(q,&q,extent,token);
                (void) FormatLocaleString(name,MagickPathExtent,"%s",token);
                for (p=q; *q != '\0'; )
                {
                  GetNextToken(q,&q,extent,token);
                  if (LocaleCompare(token,"pop") != 0)
                    continue;
                  GetNextToken(q,(const char **) NULL,extent,token);
                  if (LocaleCompare(token,"clip-path") != 0)
                    continue;
                  break;
                }
                (void) CopyMagickString(token,p,(size_t) (q-p-4+1));
                (void) SetImageArtifact(image,name,token);
                GetNextToken(q,&q,extent,token);
                break;
              }
            if (LocaleCompare("gradient",token) == 0)
              {
                char
                  key[2*MagickPathExtent],
                  name[MagickPathExtent],
                  type[MagickPathExtent];

                SegmentInfo
                  segment;

                GetNextToken(q,&q,extent,token);
                (void) CopyMagickString(name,token,MagickPathExtent);
                GetNextToken(q,&q,extent,token);
                (void) CopyMagickString(type,token,MagickPathExtent);
                GetNextToken(q,&q,extent,token);
                segment.x1=StringToDouble(token,(char **) NULL);
                GetNextToken(q,&q,extent,token);
                if (*token == ',')
                  GetNextToken(q,&q,extent,token);
                segment.y1=StringToDouble(token,(char **) NULL);
                GetNextToken(q,&q,extent,token);
                if (*token == ',')
                  GetNextToken(q,&q,extent,token);
                segment.x2=StringToDouble(token,(char **) NULL);
                GetNextToken(q,&q,extent,token);
                if (*token == ',')
                  GetNextToken(q,&q,extent,token);
                segment.y2=StringToDouble(token,(char **) NULL);
                if (LocaleCompare(type,"radial") == 0)
                  {
                    GetNextToken(q,&q,extent,token);
                    if (*token == ',')
                      GetNextToken(q,&q,extent,token);
                  }
                for (p=q; *q != '\0'; )
                {
                  GetNextToken(q,&q,extent,token);
                  if (LocaleCompare(token,"pop") != 0)
                    continue;
                  GetNextToken(q,(const char **) NULL,extent,token);
                  if (LocaleCompare(token,"gradient") != 0)
                    continue;
                  break;
                }
                (void) CopyMagickString(token,p,(size_t) (q-p-4+1));
                bounds.x1=graphic_context[n]->affine.sx*segment.x1+
                  graphic_context[n]->affine.ry*segment.y1+
                  graphic_context[n]->affine.tx;
                bounds.y1=graphic_context[n]->affine.rx*segment.x1+
                  graphic_context[n]->affine.sy*segment.y1+
                  graphic_context[n]->affine.ty;
                bounds.x2=graphic_context[n]->affine.sx*segment.x2+
                  graphic_context[n]->affine.ry*segment.y2+
                  graphic_context[n]->affine.tx;
                bounds.y2=graphic_context[n]->affine.rx*segment.x2+
                  graphic_context[n]->affine.sy*segment.y2+
                  graphic_context[n]->affine.ty;
                (void) FormatLocaleString(key,MagickPathExtent,"%s",name);
                (void) SetImageArtifact(image,key,token);
                (void) FormatLocaleString(key,MagickPathExtent,"%s-type",name);
                (void) SetImageArtifact(image,key,type);
                (void) FormatLocaleString(key,MagickPathExtent,"%s-geometry",name);
                (void) FormatLocaleString(geometry,MagickPathExtent,
                  "%gx%g%+.15g%+.15g",
                  MagickMax(fabs(bounds.x2-bounds.x1+1.0),1.0),
                  MagickMax(fabs(bounds.y2-bounds.y1+1.0),1.0),
                  bounds.x1,bounds.y1);
                (void) SetImageArtifact(image,key,geometry);
                GetNextToken(q,&q,extent,token);
                break;
              }
            if (LocaleCompare("pattern",token) == 0)
              {
                char
                  key[2*MagickPathExtent],
                  name[MagickPathExtent];

                RectangleInfo
                  pattern_bounds;

                GetNextToken(q,&q,extent,token);
                (void) CopyMagickString(name,token,MagickPathExtent);
                GetNextToken(q,&q,extent,token);
                pattern_bounds.x=(ssize_t) ceil(StringToDouble(token,
                  (char **) NULL)-0.5);
                GetNextToken(q,&q,extent,token);
                if (*token == ',')
                  GetNextToken(q,&q,extent,token);
                pattern_bounds.y=(ssize_t) ceil(StringToDouble(token,
                  (char **) NULL)-0.5);
                GetNextToken(q,&q,extent,token);
                if (*token == ',')
                  GetNextToken(q,&q,extent,token);
                pattern_bounds.width=(size_t) floor(StringToDouble(token,
                  (char **) NULL)+0.5);
                GetNextToken(q,&q,extent,token);
                if (*token == ',')
                  GetNextToken(q,&q,extent,token);
                pattern_bounds.height=(size_t) floor(StringToDouble(token,
                  (char **) NULL)+0.5);
                for (p=q; *q != '\0'; )
                {
                  GetNextToken(q,&q,extent,token);
                  if (LocaleCompare(token,"pop") != 0)
                    continue;
                  GetNextToken(q,(const char **) NULL,extent,token);
                  if (LocaleCompare(token,"pattern") != 0)
                    continue;
                  break;
                }
                (void) CopyMagickString(token,p,(size_t) (q-p-4+1));
                (void) FormatLocaleString(key,MagickPathExtent,"%s",name);
                (void) SetImageArtifact(image,key,token);
                (void) FormatLocaleString(key,MagickPathExtent,"%s-geometry",
                  name);
                (void) FormatLocaleString(geometry,MagickPathExtent,
                  "%.20gx%.20g%+.20g%+.20g",(double)pattern_bounds.width,
                  (double)pattern_bounds.height,(double)pattern_bounds.x,
                  (double)pattern_bounds.y);
                (void) SetImageArtifact(image,key,geometry);
                GetNextToken(q,&q,extent,token);
                break;
              }
            if (LocaleCompare("graphic-context",token) == 0)
              {
                n++;
                graphic_context=(DrawInfo **) ResizeQuantumMemory(
                  graphic_context,(size_t) (n+1),sizeof(*graphic_context));
                if (graphic_context == (DrawInfo **) NULL)
                  {
                    (void) ThrowMagickException(exception,GetMagickModule(),
                      ResourceLimitError,"MemoryAllocationFailed","`%s'",
                      image->filename);
                    break;
                  }
                graphic_context[n]=CloneDrawInfo((ImageInfo *) NULL,
                  graphic_context[n-1]);
                break;
              }
            if (LocaleCompare("defs",token) == 0)
              break;
            status=MagickFalse;
            break;
          }
        status=MagickFalse;
        break;
      }
      case 'r':
      case 'R':
      {
        if (LocaleCompare("rectangle",keyword) == 0)
          {
            primitive_type=RectanglePrimitive;
            break;
          }
        if (LocaleCompare("rotate",keyword) == 0)
          {
            GetNextToken(q,&q,extent,token);
            angle=StringToDouble(token,(char **) NULL);
            affine.sx=cos(DegreesToRadians(fmod((double) angle,360.0)));
            affine.rx=sin(DegreesToRadians(fmod((double) angle,360.0)));
            affine.ry=(-sin(DegreesToRadians(fmod((double) angle,360.0))));
            affine.sy=cos(DegreesToRadians(fmod((double) angle,360.0)));
            break;
          }
        if (LocaleCompare("roundRectangle",keyword) == 0)
          {
            primitive_type=RoundRectanglePrimitive;
            break;
          }
        status=MagickFalse;
        break;
      }
      case 's':
      case 'S':
      {
        if (LocaleCompare("scale",keyword) == 0)
          {
            GetNextToken(q,&q,extent,token);
            affine.sx=StringToDouble(token,(char **) NULL);
            GetNextToken(q,&q,extent,token);
            if (*token == ',')
              GetNextToken(q,&q,extent,token);
            affine.sy=StringToDouble(token,(char **) NULL);
            break;
          }
        if (LocaleCompare("skewX",keyword) == 0)
          {
            GetNextToken(q,&q,extent,token);
            angle=StringToDouble(token,(char **) NULL);
            affine.ry=sin(DegreesToRadians(angle));
            break;
          }
        if (LocaleCompare("skewY",keyword) == 0)
          {
            GetNextToken(q,&q,extent,token);
            angle=StringToDouble(token,(char **) NULL);
            affine.rx=(-tan(DegreesToRadians(angle)/2.0));
            break;
          }
        if (LocaleCompare("stop-color",keyword) == 0)
          {
            PixelInfo
              stop_color;

            number_stops++;
            if (number_stops == 1)
              stops=(StopInfo *) AcquireQuantumMemory(2,sizeof(*stops));
            else if (number_stops > 2)
              stops=(StopInfo *) ResizeQuantumMemory(stops,number_stops,
                sizeof(*stops));
            if (stops == (StopInfo *) NULL)
              {
                (void) ThrowMagickException(exception,GetMagickModule(),
                  ResourceLimitError,"MemoryAllocationFailed","`%s'",
                  image->filename);
                break;
              }
            GetNextToken(q,&q,extent,token);
            (void) QueryColorCompliance(token,AllCompliance,&stop_color,
              exception);
            stops[number_stops-1].color=stop_color;
            GetNextToken(q,&q,extent,token);
            stops[number_stops-1].offset=StringToDouble(token,(char **) NULL);
            break;
          }
        if (LocaleCompare("stroke",keyword) == 0)
          {
            GetNextToken(q,&q,extent,token);
            (void) FormatLocaleString(pattern,MagickPathExtent,"%s",token);
            if (GetImageArtifact(image,pattern) != (const char *) NULL)
              (void) DrawPatternPath(image,draw_info,token,
                &graphic_context[n]->stroke_pattern,exception);
            else
              {
                status&=QueryColorCompliance(token,AllCompliance,
                  &graphic_context[n]->stroke,exception);
                if (status == MagickFalse)
                  {
                    ImageInfo
                      *pattern_info;

                    pattern_info=AcquireImageInfo();
                    (void) CopyMagickString(pattern_info->filename,token,
                      MagickPathExtent);
                    graphic_context[n]->stroke_pattern=ReadImage(pattern_info,
                      exception);
                    CatchException(exception);
                    pattern_info=DestroyImageInfo(pattern_info);
                  }
              }
            break;
          }
        if (LocaleCompare("stroke-antialias",keyword) == 0)
          {
            GetNextToken(q,&q,extent,token);
            graphic_context[n]->stroke_antialias=
              StringToLong(token) != 0 ? MagickTrue : MagickFalse;
            break;
          }
        if (LocaleCompare("stroke-dasharray",keyword) == 0)
          {
            if (graphic_context[n]->dash_pattern != (double *) NULL)
              graphic_context[n]->dash_pattern=(double *)
                RelinquishMagickMemory(graphic_context[n]->dash_pattern);
            if (IsPoint(q) != MagickFalse)
              {
                const char
                  *r;

                r=q;
                GetNextToken(r,&r,extent,token);
                if (*token == ',')
                  GetNextToken(r,&r,extent,token);
                for (x=0; IsPoint(token) != MagickFalse; x++)
                {
                  GetNextToken(r,&r,extent,token);
                  if (*token == ',')
                    GetNextToken(r,&r,extent,token);
                }
                graphic_context[n]->dash_pattern=(double *)
                  AcquireQuantumMemory((size_t) (2UL*x+1UL),
                  sizeof(*graphic_context[n]->dash_pattern));
                if (graphic_context[n]->dash_pattern == (double *) NULL)
                  {
                    (void) ThrowMagickException(exception,GetMagickModule(),
                      ResourceLimitError,"MemoryAllocationFailed","`%s'",
                      image->filename);
                    break;
                  }
                for (j=0; j < x; j++)
                {
                  GetNextToken(q,&q,extent,token);
                  if (*token == ',')
                    GetNextToken(q,&q,extent,token);
                  graphic_context[n]->dash_pattern[j]=StringToDouble(token,
                    (char **) NULL);
                  if (graphic_context[n]->dash_pattern[j] < 0.0)
                    status=MagickFalse;
                }
                if ((x & 0x01) != 0)
                  for ( ; j < (2*x); j++)
                    graphic_context[n]->dash_pattern[j]=
                      graphic_context[n]->dash_pattern[j-x];
                graphic_context[n]->dash_pattern[j]=0.0;
                break;
              }
            GetNextToken(q,&q,extent,token);
            break;
          }
        if (LocaleCompare("stroke-dashoffset",keyword) == 0)
          {
            GetNextToken(q,&q,extent,token);
            graphic_context[n]->dash_offset=StringToDouble(token,
              (char **) NULL);
            break;
          }
        if (LocaleCompare("stroke-linecap",keyword) == 0)
          {
            ssize_t
              linecap;

            GetNextToken(q,&q,extent,token);
            linecap=ParseCommandOption(MagickLineCapOptions,MagickFalse,token);
            if (linecap == -1)
              status=MagickFalse;
            else
              graphic_context[n]->linecap=(LineCap) linecap;
            break;
          }
        if (LocaleCompare("stroke-linejoin",keyword) == 0)
          {
            ssize_t
              linejoin;

            GetNextToken(q,&q,extent,token);
            linejoin=ParseCommandOption(MagickLineJoinOptions,MagickFalse,
              token);
            if (linejoin == -1)
              status=MagickFalse;
            else
              graphic_context[n]->linejoin=(LineJoin) linejoin;
            break;
          }
        if (LocaleCompare("stroke-miterlimit",keyword) == 0)
          {
            GetNextToken(q,&q,extent,token);
            graphic_context[n]->miterlimit=StringToUnsignedLong(token);
            break;
          }
        if (LocaleCompare("stroke-opacity",keyword) == 0)
          {
            GetNextToken(q,&q,extent,token);
            factor=strchr(token,'%') != (char *) NULL ? 0.01 : 1.0;
            graphic_context[n]->stroke.alpha=(double) QuantumRange*
              factor*StringToDouble(token,(char **) NULL);
            break;
          }
        if (LocaleCompare("stroke-width",keyword) == 0)
          {
            GetNextToken(q,&q,extent,token);
            graphic_context[n]->stroke_width=StringToDouble(token,
              (char **) NULL);
            break;
          }
        status=MagickFalse;
        break;
      }
      case 't':
      case 'T':
      {
        if (LocaleCompare("text",keyword) == 0)
          {
            primitive_type=TextPrimitive;
            break;
          }
        if (LocaleCompare("text-align",keyword) == 0)
          {
            ssize_t
              align;

            GetNextToken(q,&q,extent,token);
            align=ParseCommandOption(MagickAlignOptions,MagickFalse,token);
            if (align == -1)
              status=MagickFalse;
            else
              graphic_context[n]->align=(AlignType) align;
            break;
          }
        if (LocaleCompare("text-anchor",keyword) == 0)
          {
            ssize_t
              align;

            GetNextToken(q,&q,extent,token);
            align=ParseCommandOption(MagickAlignOptions,MagickFalse,token);
            if (align == -1)
              status=MagickFalse;
            else
              graphic_context[n]->align=(AlignType) align;
            break;
          }
        if (LocaleCompare("text-antialias",keyword) == 0)
          {
            GetNextToken(q,&q,extent,token);
            graphic_context[n]->text_antialias=
              StringToLong(token) != 0 ? MagickTrue : MagickFalse;
            break;
          }
        if (LocaleCompare("text-undercolor",keyword) == 0)
          {
            GetNextToken(q,&q,extent,token);
            (void) QueryColorCompliance(token,AllCompliance,
              &graphic_context[n]->undercolor,exception);
            break;
          }
        if (LocaleCompare("translate",keyword) == 0)
          {
            GetNextToken(q,&q,extent,token);
            affine.tx=StringToDouble(token,(char **) NULL);
            GetNextToken(q,&q,extent,token);
            if (*token == ',')
              GetNextToken(q,&q,extent,token);
            affine.ty=StringToDouble(token,(char **) NULL);
            break;
          }
        status=MagickFalse;
        break;
      }
      case 'v':
      case 'V':
      {
        if (LocaleCompare("viewbox",keyword) == 0)
          {
            GetNextToken(q,&q,extent,token);
            graphic_context[n]->viewbox.x=(ssize_t) ceil(StringToDouble(token,
              (char **) NULL)-0.5);
            GetNextToken(q,&q,extent,token);
            if (*token == ',')
              GetNextToken(q,&q,extent,token);
            graphic_context[n]->viewbox.y=(ssize_t) ceil(StringToDouble(token,
              (char **) NULL)-0.5);
            GetNextToken(q,&q,extent,token);
            if (*token == ',')
              GetNextToken(q,&q,extent,token);
            graphic_context[n]->viewbox.width=(size_t) floor(StringToDouble(
              token,(char **) NULL)+0.5);
            GetNextToken(q,&q,extent,token);
            if (*token == ',')
              GetNextToken(q,&q,extent,token);
            graphic_context[n]->viewbox.height=(size_t) floor(StringToDouble(
              token,(char **) NULL)+0.5);
            break;
          }
        status=MagickFalse;
        break;
      }
      default:
      {
        status=MagickFalse;
        break;
      }
    }
    if (status == MagickFalse)
      break;
    if ((affine.sx != 1.0) || (affine.rx != 0.0) || (affine.ry != 0.0) ||
        (affine.sy != 1.0) || (affine.tx != 0.0) || (affine.ty != 0.0))
      {
        graphic_context[n]->affine.sx=current.sx*affine.sx+current.ry*affine.rx;
        graphic_context[n]->affine.rx=current.rx*affine.sx+current.sy*affine.rx;
        graphic_context[n]->affine.ry=current.sx*affine.ry+current.ry*affine.sy;
        graphic_context[n]->affine.sy=current.rx*affine.ry+current.sy*affine.sy;
        graphic_context[n]->affine.tx=current.sx*affine.tx+current.ry*affine.ty+
          current.tx;
        graphic_context[n]->affine.ty=current.rx*affine.tx+current.sy*affine.ty+
          current.ty;
      }
    if (primitive_type == UndefinedPrimitive)
      {
        if (*q == '\0')
          {
            if (number_stops > 1)
              {
                GradientType
                  type;

              type=LinearGradient;
              if (draw_info->gradient.type == RadialGradient)
                type=RadialGradient;
              (void) GradientImage(image,type,PadSpread,stops,number_stops,
                 exception);
             }
           if (number_stops > 0)
             stops=(StopInfo *) RelinquishMagickMemory(stops);
          }
        if (image->debug != MagickFalse)
          (void) LogMagickEvent(DrawEvent,GetMagickModule(),"  %.*s",
            (int) (q-p),p);
        continue;
      }
    /*
      Parse the primitive attributes.
    */
    i=0;
    j=0;
    primitive_info[0].point.x=0.0;
    primitive_info[0].point.y=0.0;
    for (x=0; *q != '\0'; x++)
    {
      /*
        Define points.
      */
      if (IsPoint(q) == MagickFalse)
        break;
      GetNextToken(q,&q,extent,token);
      point.x=StringToDouble(token,(char **) NULL);
      GetNextToken(q,&q,extent,token);
      if (*token == ',')
        GetNextToken(q,&q,extent,token);
      point.y=StringToDouble(token,(char **) NULL);
      GetNextToken(q,(const char **) NULL,extent,token);
      if (*token == ',')
        GetNextToken(q,&q,extent,token);
      primitive_info[i].primitive=primitive_type;
      primitive_info[i].point=point;
      primitive_info[i].coordinates=0;
      primitive_info[i].method=FloodfillMethod;
      i++;
      if (i < (ssize_t) number_points)
        continue;
      number_points<<=1;
      primitive_info=(PrimitiveInfo *) ResizeQuantumMemory(primitive_info,
        (size_t) number_points,sizeof(*primitive_info));
      if (primitive_info == (PrimitiveInfo *) NULL)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),
            ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
          break;
        }
    }
    primitive_info[j].primitive=primitive_type;
    primitive_info[j].coordinates=(size_t) x;
    primitive_info[j].method=FloodfillMethod;
    primitive_info[j].text=(char *) NULL;
    /*
      Circumscribe primitive within a circle.
    */
    bounds.x1=primitive_info[j].point.x;
    bounds.y1=primitive_info[j].point.y;
    bounds.x2=primitive_info[j].point.x;
    bounds.y2=primitive_info[j].point.y;
    for (k=1; k < (ssize_t) primitive_info[j].coordinates; k++)
    {
      point=primitive_info[j+k].point;
      if (point.x < bounds.x1)
        bounds.x1=point.x;
      if (point.y < bounds.y1)
        bounds.y1=point.y;
      if (point.x > bounds.x2)
        bounds.x2=point.x;
      if (point.y > bounds.y2)
        bounds.y2=point.y;
    }
    /*
      Speculate how many points our primitive might consume.
    */
    length=primitive_info[j].coordinates;
    switch (primitive_type)
    {
      case RectanglePrimitive:
      {
        length*=5;
        break;
      }
      case RoundRectanglePrimitive:
      {
        double
          alpha,
          beta,
          radius;

        alpha=bounds.x2-bounds.x1;
        beta=bounds.y2-bounds.y1;
        radius=hypot((double) alpha,(double) beta);
        length*=5;
        length+=2*((size_t) ceil((double) MagickPI*radius))+6*BezierQuantum+360;
        break;
      }
      case BezierPrimitive:
      {
        if (primitive_info[j].coordinates > 107)
          (void) ThrowMagickException(exception,GetMagickModule(),DrawError,
            "TooManyBezierCoordinates","`%s'",token);
        length=BezierQuantum*primitive_info[j].coordinates;
        break;
      }
      case PathPrimitive:
      {
        char
          *s,
          *t;

        GetNextToken(q,&q,extent,token);
        length=1;
        t=token;
        for (s=token; *s != '\0'; s=t)
        {
          double
            value;

          value=StringToDouble(s,&t);
          (void) value;
          if (s == t)
            {
              t++;
              continue;
            }
          length++;
        }
        length=length*BezierQuantum/2;
        break;
      }
      case CirclePrimitive:
      case ArcPrimitive:
      case EllipsePrimitive:
      {
        double
          alpha,
          beta,
          radius;

        alpha=bounds.x2-bounds.x1;
        beta=bounds.y2-bounds.y1;
        radius=hypot((double) alpha,(double) beta);
        length=2*((size_t) ceil((double) MagickPI*radius))+6*BezierQuantum+360;
        break;
      }
      default:
        break;
    }
    if ((size_t) (i+length) >= number_points)
      {
        /*
          Resize based on speculative points required by primitive.
        */
        number_points+=length+1;
        primitive_info=(PrimitiveInfo *) ResizeQuantumMemory(primitive_info,
          (size_t) number_points,sizeof(*primitive_info));
        if (primitive_info == (PrimitiveInfo *) NULL)
          {
            (void) ThrowMagickException(exception,GetMagickModule(),
              ResourceLimitError,"MemoryAllocationFailed","`%s'",
              image->filename);
            break;
          }
      }
    switch (primitive_type)
    {
      case PointPrimitive:
      default:
      {
        if (primitive_info[j].coordinates != 1)
          {
            status=MagickFalse;
            break;
          }
        TracePoint(primitive_info+j,primitive_info[j].point);
        i=(ssize_t) (j+primitive_info[j].coordinates);
        break;
      }
      case LinePrimitive:
      {
        if (primitive_info[j].coordinates != 2)
          {
            status=MagickFalse;
            break;
          }
        TraceLine(primitive_info+j,primitive_info[j].point,
          primitive_info[j+1].point);
        i=(ssize_t) (j+primitive_info[j].coordinates);
        break;
      }
      case RectanglePrimitive:
      {
        if (primitive_info[j].coordinates != 2)
          {
            status=MagickFalse;
            break;
          }
        TraceRectangle(primitive_info+j,primitive_info[j].point,
          primitive_info[j+1].point);
        i=(ssize_t) (j+primitive_info[j].coordinates);
        break;
      }
      case RoundRectanglePrimitive:
      {
        if (primitive_info[j].coordinates != 3)
          {
            status=MagickFalse;
            break;
          }
        TraceRoundRectangle(primitive_info+j,primitive_info[j].point,
          primitive_info[j+1].point,primitive_info[j+2].point);
        i=(ssize_t) (j+primitive_info[j].coordinates);
        break;
      }
      case ArcPrimitive:
      {
        if (primitive_info[j].coordinates != 3)
          {
            primitive_type=UndefinedPrimitive;
            break;
          }
        TraceArc(primitive_info+j,primitive_info[j].point,
          primitive_info[j+1].point,primitive_info[j+2].point);
        i=(ssize_t) (j+primitive_info[j].coordinates);
        break;
      }
      case EllipsePrimitive:
      {
        if (primitive_info[j].coordinates != 3)
          {
            status=MagickFalse;
            break;
          }
        TraceEllipse(primitive_info+j,primitive_info[j].point,
          primitive_info[j+1].point,primitive_info[j+2].point);
        i=(ssize_t) (j+primitive_info[j].coordinates);
        break;
      }
      case CirclePrimitive:
      {
        if (primitive_info[j].coordinates != 2)
          {
            status=MagickFalse;
            break;
          }
        TraceCircle(primitive_info+j,primitive_info[j].point,
          primitive_info[j+1].point);
        i=(ssize_t) (j+primitive_info[j].coordinates);
        break;
      }
      case PolylinePrimitive:
        break;
      case PolygonPrimitive:
      {
        primitive_info[i]=primitive_info[j];
        primitive_info[i].coordinates=0;
        primitive_info[j].coordinates++;
        i++;
        break;
      }
      case BezierPrimitive:
      {
        if (primitive_info[j].coordinates < 3)
          {
            status=MagickFalse;
            break;
          }
        TraceBezier(primitive_info+j,primitive_info[j].coordinates);
        i=(ssize_t) (j+primitive_info[j].coordinates);
        break;
      }
      case PathPrimitive:
      {
        i=(ssize_t) (j+TracePath(primitive_info+j,token));
        break;
      }
      case AlphaPrimitive:
      case ColorPrimitive:
      {
        ssize_t
          method;

        if (primitive_info[j].coordinates != 1)
          {
            status=MagickFalse;
            break;
          }
        GetNextToken(q,&q,extent,token);
        method=ParseCommandOption(MagickMethodOptions,MagickFalse,token);
        if (method == -1)
          status=MagickFalse;
        else
          primitive_info[j].method=(PaintMethod) method;
        break;
      }
      case TextPrimitive:
      {
        if (primitive_info[j].coordinates != 1)
          {
            status=MagickFalse;
            break;
          }
        if (*token != ',')
          GetNextToken(q,&q,extent,token);
        primitive_info[j].text=AcquireString(token);
        break;
      }
      case ImagePrimitive:
      {
        if (primitive_info[j].coordinates != 2)
          {
            status=MagickFalse;
            break;
          }
        GetNextToken(q,&q,extent,token);
        primitive_info[j].text=AcquireString(token);
        break;
      }
    }
    if (primitive_info == (PrimitiveInfo *) NULL)
      break;
    if (image->debug != MagickFalse)
      (void) LogMagickEvent(DrawEvent,GetMagickModule(),"  %.*s",(int) (q-p),p);
    if (status == MagickFalse)
      break;
    primitive_info[i].primitive=UndefinedPrimitive;
    if (i == 0)
      continue;
    /*
      Transform points.
    */
    for (i=0; primitive_info[i].primitive != UndefinedPrimitive; i++)
    {
      point=primitive_info[i].point;
      primitive_info[i].point.x=graphic_context[n]->affine.sx*point.x+
        graphic_context[n]->affine.ry*point.y+graphic_context[n]->affine.tx;
      primitive_info[i].point.y=graphic_context[n]->affine.rx*point.x+
        graphic_context[n]->affine.sy*point.y+graphic_context[n]->affine.ty;
      point=primitive_info[i].point;
      if (point.x < graphic_context[n]->bounds.x1)
        graphic_context[n]->bounds.x1=point.x;
      if (point.y < graphic_context[n]->bounds.y1)
        graphic_context[n]->bounds.y1=point.y;
      if (point.x > graphic_context[n]->bounds.x2)
        graphic_context[n]->bounds.x2=point.x;
      if (point.y > graphic_context[n]->bounds.y2)
        graphic_context[n]->bounds.y2=point.y;
      if (primitive_info[i].primitive == ImagePrimitive)
        break;
      if (i >= (ssize_t) number_points)
        ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
    }
    if (graphic_context[n]->render != MagickFalse)
      {
        if ((n != 0) && (graphic_context[n]->clip_mask != (char *) NULL) &&
            (LocaleCompare(graphic_context[n]->clip_mask,
             graphic_context[n-1]->clip_mask) != 0))
          status&=DrawClipPath(image,graphic_context[n],
            graphic_context[n]->clip_mask,exception);
        status&=DrawPrimitive(image,graphic_context[n],primitive_info,
          exception);
      }
    if (primitive_info->text != (char *) NULL)
      primitive_info->text=(char *) RelinquishMagickMemory(
        primitive_info->text);
    proceed=SetImageProgress(image,RenderImageTag,q-primitive,(MagickSizeType)
      primitive_extent);
    if (proceed == MagickFalse)
      break;
    if (status == 0)
      break;
  }
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),"end draw-image");
  /*
    Relinquish resources.
  */
  token=DestroyString(token);
  if (primitive_info != (PrimitiveInfo *) NULL)
    primitive_info=(PrimitiveInfo *) RelinquishMagickMemory(primitive_info);
  primitive=DestroyString(primitive);
  for ( ; n >= 0; n--)
    graphic_context[n]=DestroyDrawInfo(graphic_context[n]);
  graphic_context=(DrawInfo **) RelinquishMagickMemory(graphic_context);
  if (status == MagickFalse)
    ThrowBinaryException(DrawError,"NonconformingDrawingPrimitiveDefinition",
      keyword);
  return(status != 0 ? MagickTrue : MagickFalse);
}