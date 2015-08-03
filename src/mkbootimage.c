/* Copyright (c) 2013-2015, Antmicro Ltd
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <argp.h>

#include "bif.h"
#include "bootrom.h"

/* Prepare global variables for arg parser */
const char *argp_program_version = MKBOOTIMAGE_VER;
static char doc[] = "Generate bootloader images for Xilinx Zynq based platforms.";
static char args_doc[] = "<input_bif_file> <output_bin_file>";

/* Prapare struct for holding parsed arguments */
struct arguments {
  char *bif_filename;
  char *bin_filename;
};

/* Define argument parser */
static error_t argp_parser(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = state->input;

  switch (key) {
  case ARGP_KEY_ARG:
    switch(state->arg_num) {
    case 0:
      arguments->bif_filename = arg;
      break;
    case 1:
      arguments->bin_filename = arg;
      break;
    default:
      argp_usage(state);
    }
    break;
  case ARGP_KEY_END:
    if (state->arg_num < 2)
      argp_usage (state);
    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

/* Finally initialize argp struct */
static struct argp argp = { 0, argp_parser, args_doc, doc, 0, 0, 0 };

/* Declare the main function */
int main(int argc, char *argv[]) {
  FILE *ofile;
  uint32_t ofile_size;
  uint32_t *file_data;
  struct arguments arguments;
  bif_cfg_t cfg;
  int ret;
  int i;

  /* Parse program arguments */
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  init_bif_cfg(&cfg);

  ret = parse_bif(arguments.bif_filename, &cfg);
  if (ret != BIF_SUCCESS)
    fprintf(stderr, "Could not parse %s file.\n", arguments.bif_filename);

  printf("Nodes found in the %s file:\n", arguments.bif_filename);
  for (i = 0; i < cfg.nodes_num; i++) {
    printf(" %s", cfg.nodes[i].fname);
    if (cfg.nodes[i].bootloader)
      printf(" (bootloader)\n");
    else
      printf("\n");
    if (cfg.nodes[i].load)
      printf("  load:   %08x\n", cfg.nodes[i].load);
    if (cfg.nodes[i].offset)
      printf("  offset: %08x\n", cfg.nodes[i].offset);
  }

  /* Generate bin file */
  file_data = malloc (sizeof *file_data * 10000000);

  ret = create_boot_image(file_data, &cfg, &ofile_size);

  if (ret != BOOTROM_SUCCESS) { /* Error */
    free(file_data);
    return ofile_size;
  }

  ofile = fopen(arguments.bin_filename, "wb");

  if (ofile == NULL ) {
    fprintf(stderr, "Could not open output file: %s\n", arguments.bin_filename);
    return EXIT_FAILURE;
  }

  fwrite(file_data, sizeof(uint32_t), ofile_size, ofile);

  fclose(ofile);
  free(file_data);
  deinit_bif_cfg(&cfg);

  printf("All done, quitting\n");
  return EXIT_SUCCESS;
}

