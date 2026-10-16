Files for the installable NSP build (`tools/build.ps1 nsp`):

- `bm64.npdm.json`: process permissions, made from `hbl.json` of
  [nx-hbloader](https://github.com/switchbrew/nx-hbloader) at commit 82b95122c5ae8dc059bf23893ba7623c72c86773, which is
  what the `.nro` already runs under in title override mode. Changed: name, title ID 0100BB6400000000 (as program_id, the current npdmtool field names) and
  application type (1, a regular application).

  nx-hbloader is ISC licensed: Copyright 2017-2018 nx-hbloader Authors. Permission to use, copy, modify, and/or
  distribute this software for any purpose with or without fee is hereby granted, provided that the above copyright
  notice and this permission notice appear in all copies.

The NSP is packed with hacBrewPack ([tiliarou fork](https://github.com/tiliarou/hacBrewPack), pinned to commit
1d23a9db8e70e8cddede4be4dd627e6811e2fc8d; the original repository no longer exists). It needs your console's
`prod.keys`, so `tools/build-nsp.sh pack` runs in a container without network access.
