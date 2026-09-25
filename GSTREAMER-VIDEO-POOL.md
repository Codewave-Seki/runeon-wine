# GStreamer video pool fallback

[English](GSTREAMER-VIDEO-POOL.md) | [Simplified Chinese](GSTREAMER-VIDEO-POOL.zh-CN.md) | [Japanese](GSTREAMER-VIDEO-POOL.ja.md)

Candidate `cx26.3-wine11.0-runeon.11` adds downstream patch `0106-winegstreamer-unfixed-caps-pool.patch`.

## Cause and implementation

`wg_video_buffer_pool_create` ignored the result of `gst_video_info_from_caps`. An allocation query may carry caps that cannot be described yet; the zeroed video info was then used and the query was answered with a pool advertising a size of zero, so a decoder could wait for a buffer that can never be allocated. The patch checks the result, releases the pool and returns NULL. The existing caller already treats NULL as "no pool offered" and falls back to GStreamer's default allocation.

This is [dappermint/winecx `9fcbfbe`](https://github.com/dappermint/winecx/commit/9fcbfbe12bd9e3d03787909d9488009dcdba0a90), an LGPL-2.1-or-later change to a Wine source file, adapted only in context: the 11.0 base lacks the later `output_plane_stride` argument. WineHQ master still ignores this return value, so it is not a release backport. No game-specific logic is included.

## Validation

The patch applies to the pinned base and is covered by `scripts/static-check.sh` and `scripts/integration-check.sh`. The winegstreamer unix library is compiled by the full Wine build. There is no local reproducer for undescribable caps, so this candidate only claims that the unchanged path keeps working; product validation should play existing video paths without regression. It does not claim that any game's video is fixed.
