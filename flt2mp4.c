#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <png.h>
#include "cmap.h"

#define MAXLINE 256
#define NCOLORS 256

// -----------------------------------------------------------------------------

typedef struct {
    int ncols, nrows;
    double xllcorner, yllcorner, cellsize;
    double nodata;
} RasterHeader;

int read_header(const char *fname, RasterHeader *hdr) {
    FILE *fp = fopen(fname, "r");
    if (!fp) { perror("Header open failed"); return 0; }
    char key[MAXLINE]; double val;
    while (fscanf(fp, "%s %lf", key, &val) == 2) {
        if (strcmp(key, "ncols") == 0) hdr->ncols = (int)val;
        else if (strcmp(key, "nrows") == 0) hdr->nrows = (int)val;
        else if (strcmp(key, "xllcorner") == 0) hdr->xllcorner = val;
        else if (strcmp(key, "yllcorner") == 0) hdr->yllcorner = val;
        else if (strcmp(key, "cellsize") == 0) hdr->cellsize = val;
        else if (strcmp(key, "NODATA_value") == 0) hdr->nodata = val;
    }
    fclose(fp); return 1;
}

float *read_flt_colmajor(const char *fname, int nrows, int ncols) {
    FILE *fp = fopen(fname, "rb");
    if (!fp) { perror("Raster open failed"); return NULL; }
    size_t n = (size_t)nrows * ncols;
    float *raw = (float*)malloc(n*sizeof(float));
    float *data = (float*)malloc(n*sizeof(float));
    fread(raw, sizeof(float), n, fp); fclose(fp);
    for (int col=0; col<ncols; col++)
        for (int row=0; row<nrows; row++)
            data[row*ncols+col] = raw[col*nrows+row];
    free(raw); return data;
}

int write_png(const char *fname, unsigned char *rgb, int width, int height) {
    FILE *fp = fopen(fname,"wb"); if (!fp){ perror("PNG open failed"); return 0;}
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
    if (!png) return 0;
    png_infop info = png_create_info_struct(png); if (!info) return 0;
    if (setjmp(png_jmpbuf(png))) return 0;
    png_init_io(png,fp);
    png_set_IHDR(png,info,width,height,8,PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE,PNG_COMPRESSION_TYPE_BASE,PNG_FILTER_TYPE_BASE);
    png_write_info(png,info);
    png_bytep row_ptrs[height];
    for(int y=0;y<height;y++) row_ptrs[y]=rgb+y*width*3;
    png_write_image(png,row_ptrs);
    png_write_end(png,NULL);
    fclose(fp); png_destroy_write_struct(&png,&info);
    return 1;
}

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------
int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <prefix>\n", argv[0]);
        return 1;
    }
    const char *fprefix = argv[1];
    int numsaves=10, nsaveskip=1; double dtol=0.1, pink_depth=2.0;
    char fname[MAXLINE]; RasterHeader hdr;

    // --- Read header ---
    snprintf(fname,MAXLINE,"%s/raster/%s.hd.hdr",fprefix,fprefix);
    if(!read_header(fname,&hdr)) return 1;
    printf("Loaded header: %d x %d\n",hdr.ncols,hdr.nrows);

    // --- Read topo background ---
    snprintf(fname,MAXLINE,"%s/raster/%s.z.flt",fprefix,fprefix);
    float *Z=read_flt_colmajor(fname,hdr.nrows,hdr.ncols);
    if(!Z) return 1;
    unsigned char *bg=(unsigned char*)malloc(3*hdr.nrows*hdr.ncols);
    float zmin=-5,zmax=5;
    for(int i=0;i<hdr.nrows*hdr.ncols;i++){
        float g=(Z[i]-zmin)/(zmax-zmin); if(g<0) g=0; if(g>1) g=1;
        unsigned char val=(unsigned char)(g*255);
        bg[3*i+0]=bg[3*i+1]=bg[3*i+2]=val;
    }
    free(Z);

    // --- Loop over timesteps and write PNGs ---
    for(int nsave=0;nsave<=numsaves;nsave+=nsaveskip){
        char fnsave[16]; snprintf(fnsave,sizeof(fnsave),"%04d",nsave);
        snprintf(fname,MAXLINE,"%s/raster/%s.hd.%s.flt",fprefix,fprefix,fnsave);
        printf("Loading %s\n",fname);
        float *H=read_flt_colmajor(fname,hdr.nrows,hdr.ncols);
        if(!H) continue;
        unsigned char *img=(unsigned char*)malloc(3*hdr.nrows*hdr.ncols);
        memcpy(img,bg,3*hdr.nrows*hdr.ncols);
        for(int i=0;i<hdr.nrows*hdr.ncols;i++){
            float h=H[i]; if(h<=dtol||h==hdr.nodata) continue;
            float depth_sat=h/pink_depth; if(depth_sat>1.0) depth_sat=1.0;
            int idx=(int)((NCOLORS-1)*depth_sat);
            img[3*i+0]=(unsigned char)(cmap[idx][0]*255);
            img[3*i+1]=(unsigned char)(cmap[idx][1]*255);
            img[3*i+2]=(unsigned char)(cmap[idx][2]*255);
        }
        char outname[MAXLINE]; snprintf(outname,MAXLINE,"%s/%s_%s.png",fprefix,fprefix,fnsave);
        write_png(outname,img,hdr.ncols,hdr.nrows);
        printf("Wrote %s\n",outname);
        free(H); free(img);
    }
    free(bg);

    // --- Call ffmpeg to make MP4 ---
    char input_pattern[MAXLINE];
    snprintf(input_pattern,sizeof(input_pattern),"%s/%s_%%04d.png",fprefix,fprefix);
    char output_file[MAXLINE];
    snprintf(output_file,sizeof(output_file),"%s/%s.mp4",fprefix,fprefix);
    char cmd[512];
    snprintf(cmd,sizeof(cmd),"ffmpeg -framerate 10 -i %s -pix_fmt yuv420p -y %s",
             input_pattern,output_file);
    printf("Running command: %s\n",cmd);
    int result = system(cmd);
    if(result!=0){
        fprintf(stderr,"Error: ffmpeg command failed.\n");
        return 1;
    }
    printf("Success: %s generated.\n",output_file);

    return 0;
}
