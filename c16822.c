gaussian_blur_surface (cairo_surface_t *in,
                       cairo_surface_t *out,
                       gdouble sx,
                       gdouble sy)
{
    gboolean use_box_blur;
    gint width, height;
    cairo_format_t in_format, out_format;
    gint in_stride;
    gint out_stride;
    guchar *in_data, *out_data;
    gint bpp;
    gboolean out_has_data;

    cairo_surface_flush (in);

    width = cairo_image_surface_get_width (in);
    height = cairo_image_surface_get_height (in);

    g_assert (width == cairo_image_surface_get_width (out)
              && height == cairo_image_surface_get_height (out));

    in_format = cairo_image_surface_get_format (in);
    out_format = cairo_image_surface_get_format (out);
    g_assert (in_format == out_format);
    g_assert (in_format == CAIRO_FORMAT_ARGB32
              || in_format == CAIRO_FORMAT_A8);

    if (in_format == CAIRO_FORMAT_ARGB32)
        bpp = 4;
    else if (in_format == CAIRO_FORMAT_A8)
        bpp = 1;
    else {
        g_assert_not_reached ();
        return;
    }

    in_stride = cairo_image_surface_get_stride (in);
    out_stride = cairo_image_surface_get_stride (out);

    in_data = cairo_image_surface_get_data (in);
    out_data = cairo_image_surface_get_data (out);

    if (sx < 0.0)
        sx = 0.0;

    if (sy < 0.0)
        sy = 0.0;

    /* For small radiuses, use a true gaussian kernel; otherwise use three box blurs with
     * clever offsets.
     */
    if (sx < 10.0 && sy < 10.0)
        use_box_blur = FALSE;
    else
        use_box_blur = TRUE;

    /* Bail out by just copying? */
    if ((sx == 0.0 && sy == 0.0)
        || sx > 1000 || sy > 1000) {
        cairo_t *cr;

        cr = cairo_create (out);
        cairo_set_source_surface (cr, in, 0, 0);
        cairo_paint (cr);
        cairo_destroy (cr);
        return;
    }

    if (sx != 0.0) {
        gint box_width;
        gdouble *gaussian_matrix;
        gint gaussian_matrix_len;
        int y;
        guchar *row_buffer = NULL;
        guchar *row1, *row2;

        if (use_box_blur) {
            box_width = compute_box_blur_width (sx);

            /* twice the size so we can have "two" scratch rows */
            row_buffer = g_new0 (guchar, width * bpp * 2);
            row1 = row_buffer;
            row2 = row_buffer + width * bpp;
        } else
            make_gaussian_convolution_matrix (sx, &gaussian_matrix, &gaussian_matrix_len);

        for (y = 0; y < height; y++) {
            guchar *in_row, *out_row;

            in_row = in_data + in_stride * y;
            out_row = out_data + out_stride * y;

            if (use_box_blur) {
                if (box_width % 2 != 0) {
                    /* Odd-width box blur: repeat 3 times, centered on output pixel */

                    box_blur_line (box_width, 0, in_row, row1,    width, bpp);
                    box_blur_line (box_width, 0, row1,   row2,    width, bpp);
                    box_blur_line (box_width, 0, row2,   out_row, width, bpp);
                } else {
                    /* Even-width box blur:
                     * This method is suggested by the specification for SVG.
                     * One pass with width n, centered between output and right pixel
                     * One pass with width n, centered between output and left pixel
                     * One pass with width n+1, centered on output pixel
                     */
                    box_blur_line (box_width,     -1, in_row, row1,    width, bpp);
                    box_blur_line (box_width,      1, row1,   row2,    width, bpp);
                    box_blur_line (box_width + 1,  0, row2,   out_row, width, bpp);
                }
            } else
                gaussian_blur_line (gaussian_matrix, gaussian_matrix_len, in_row, out_row, width, bpp);
        }

        if (!use_box_blur)
            g_free (gaussian_matrix);

        g_free (row_buffer);

        out_has_data = TRUE;
    } else
        out_has_data = FALSE;

    if (sy != 0.0) {
        gint box_height;
        gdouble *gaussian_matrix = NULL;
        gint gaussian_matrix_len;
        guchar *col_buffer;
        guchar *col1, *col2;
        int x;

        /* twice the size so we can have the source pixels and the blurred pixels */
        col_buffer = g_new0 (guchar, height * bpp * 2);
        col1 = col_buffer;
        col2 = col_buffer + height * bpp;

        if (use_box_blur) {
            box_height = compute_box_blur_width (sy);
        } else
            make_gaussian_convolution_matrix (sy, &gaussian_matrix, &gaussian_matrix_len);

        for (x = 0; x < width; x++) {
            if (out_has_data)
                get_column (col1, out_data, out_stride, bpp, height, x);
            else
                get_column (col1, in_data, in_stride, bpp, height, x);

            if (use_box_blur) {
                if (box_height % 2 != 0) {
                    /* Odd-width box blur */
                    box_blur_line (box_height, 0, col1, col2, height, bpp);
                    box_blur_line (box_height, 0, col2, col1, height, bpp);
                    box_blur_line (box_height, 0, col1, col2, height, bpp);
                } else {
                    /* Even-width box blur */
                    box_blur_line (box_height,     -1, col1, col2, height, bpp);
                    box_blur_line (box_height,      1, col2, col1, height, bpp);
                    box_blur_line (box_height + 1,  0, col1, col2, height, bpp);
                }
            } else
                gaussian_blur_line (gaussian_matrix, gaussian_matrix_len, col1, col2, height, bpp);

            put_column (col2, out_data, out_stride, bpp, height, x);
        }

        g_free (gaussian_matrix);
        g_free (col_buffer);
    }

    cairo_surface_mark_dirty (out);
}