// Display formatting. Kept in one place so byte/time/rate strings stay
// consistent across pages — every machine value is rendered in mono type.

export function formatBytes(bytes: number | undefined | null): string {
  if (bytes === undefined || bytes === null) return '—'
  if (bytes === 0) return '0 B'
  const units = ['B', 'KB', 'MB', 'GB', 'TB']
  const exp = Math.min(units.length - 1, Math.floor(Math.log(bytes) / Math.log(1024)))
  const value = bytes / 1024 ** exp
  const digits = exp === 0 ? 0 : value < 10 ? 1 : value < 100 ? 1 : 0
  return `${value.toFixed(digits)} ${units[exp]}`
}

export function formatRate(bytesPerSecond: number): string {
  if (!bytesPerSecond || bytesPerSecond <= 0) return '—'
  return `${formatBytes(bytesPerSecond)}/s`
}

export function formatEta(seconds: number | null): string {
  if (seconds === null || !isFinite(seconds) || seconds < 0) return '—'
  if (seconds < 1) return '即将完成'
  if (seconds < 60) return `剩余 ${Math.ceil(seconds)} 秒`
  const min = Math.floor(seconds / 60)
  const sec = Math.round(seconds % 60)
  if (min < 60) return sec > 0 ? `剩余 ${min} 分 ${sec} 秒` : `剩余 ${min} 分`
  const hours = Math.floor(min / 60)
  return `剩余 ${hours} 小时 ${min % 60} 分`
}

/** Absolute for anything older than a week, relative for recent activity. */
export function formatWhen(iso: string | null | undefined): string {
  if (!iso) return '—'
  const then = new Date(iso)
  if (Number.isNaN(then.getTime())) return '—'
  const deltaSec = (Date.now() - then.getTime()) / 1000

  if (deltaSec < 10) return '刚刚'
  if (deltaSec < 60) return `${Math.floor(deltaSec)} 秒前`
  if (deltaSec < 3600) return `${Math.floor(deltaSec / 60)} 分钟前`

  const startOfToday = new Date()
  startOfToday.setHours(0, 0, 0, 0)
  const hhmm = `${String(then.getHours()).padStart(2, '0')}:${String(then.getMinutes()).padStart(2, '0')}`

  if (then.getTime() >= startOfToday.getTime()) return `今天 ${hhmm}`
  const yesterday = startOfToday.getTime() - 86_400_000
  if (then.getTime() >= yesterday) return `昨天 ${hhmm}`
  if (deltaSec < 7 * 86_400) return `${then.getMonth() + 1}月${then.getDate()}日 ${hhmm}`
  return `${then.getFullYear()}-${String(then.getMonth() + 1).padStart(2, '0')}-${String(then.getDate()).padStart(2, '0')} ${hhmm}`
}

/** Icon id for a device, matching the sprite in components/icons.svg. */
export function deviceIcon(deviceType: string, os: string): string {
  if (deviceType === 'WEB') return 'desktop'
  if (deviceType === 'MOBILE' || /Android|iOS|iPhone/i.test(os)) return 'phone'
  if (/macOS|Mac OS/i.test(os)) return 'laptop'
  return 'desktop'
}

/** Icon id for a file, by extension. */
export function fileIcon(entry: { type: string; name: string }): string {
  if (entry.type === 'Directory') return 'folder'
  const ext = entry.name.split('.').pop()?.toLowerCase() ?? ''
  if (['png', 'jpg', 'jpeg', 'gif', 'webp', 'svg', 'bmp'].includes(ext)) return 'image'
  if (['zip', 'rar', '7z', 'tar', 'gz', 'bz2'].includes(ext)) return 'archive'
  return 'file'
}

/** Join path segments the way the server expects: relative, forward slashes. */
export function joinPath(...parts: (string | undefined | null)[]): string {
  return parts
    .filter((p): p is string => !!p && p.length > 0)
    .join('/')
    .replace(/\/{2,}/g, '/')
    .replace(/^\//, '')
}
