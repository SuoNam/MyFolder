// Transfer path description for the WEB console.
//
// The web console has exactly one channel: the server relay. A browser cannot
// open a LAN socket to a peer and cannot do UDP hole punching, so LAN and P2P
// are not shown here at all — a ranked ladder with one usable rung would be
// decoration, not information. The desktop client is the half that chooses
// between channels; this UI describes what actually happens: browser uploads to
// the server, target device downloads from the server.

import type { ServerDevice } from '@/api'

/** The two real hops of a web-originated transfer. */
export interface Hop {
  label: string
  note: string
  /** 'done' | 'active' | 'wait' — reflects the actual stage, never decoration. */
  state: 'done' | 'active' | 'wait'
}

/** One-line summary for a device card header. */
export function relayVerdict(device: ServerDevice | null): string {
  return device?.online ? '经服务器中转' : '先存服务器，等待上线'
}

/** How the transfer will reach `device`, in plain terms. */
export function relayNote(device: ServerDevice | null): string {
  return device?.online
    ? '文件先上传到服务器，目标设备在线，会立即开始下载。'
    : '文件先上传到服务器，目标设备上线后自动下载。'
}

/**
 * The two hops, given the current stage of a transfer.
 * `uploaded` = the browser finished its upload; `delivered` = the peer has it.
 */
export function hops(
  device: ServerDevice | null,
  stage: { uploaded?: boolean; delivered?: boolean } = {},
): Hop[] {
  const online = !!device?.online
  return [
    {
      label: '浏览器 → 服务器',
      note: stage.uploaded ? '已上传并校验' : '分片上传 · SHA-256 校验',
      state: stage.uploaded ? 'done' : 'active',
    },
    {
      label: '服务器 → 目标设备',
      note: stage.delivered
        ? '已送达'
        : stage.uploaded
          ? online
            ? '目标设备下载中'
            : '等待目标设备上线'
          : online
            ? '设备在线，上传完成后立即下载'
            : '设备离线，上线后自动下载',
      state: stage.delivered ? 'done' : stage.uploaded ? 'active' : 'wait',
    },
  ]
}
