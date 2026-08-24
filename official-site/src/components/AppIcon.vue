<template>
  <svg class="icon" aria-hidden="true" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round">
    <path v-if="name === 'arrow'" d="M5 12h14M14 7l5 5-5 5" />
    <path v-else-if="name === 'github'" d="M15 22v-4a4.8 4.8 0 0 0-1-3.5c3.3-.4 6.8-1.6 6.8-7A5.5 5.5 0 0 0 19.3 4 5.1 5.1 0 0 0 19.2.5S18 0 15 2a13.4 13.4 0 0 0-7 0C5-.1 3.8.5 3.8.5A5.1 5.1 0 0 0 3.7 4a5.5 5.5 0 0 0-1.5 3.8c0 5.3 3.5 6.5 6.8 7A4.8 4.8 0 0 0 8 18v4M8 19c-3 .9-3-1.5-4.2-2" />
    <path v-else-if="name === 'download'" d="M12 3v12m0 0 5-5m-5 5-5-5M5 21h14" />
    <path v-else-if="name === 'monitor'" d="M4 4h16a1 1 0 0 1 1 1v11H3V5a1 1 0 0 1 1-1Zm-1 12h18v2a1 1 0 0 1-1 1H4a1 1 0 0 1-1-1v-2Zm7 3h4" />
    <path v-else-if="name === 'shield'" d="M12 3 5 6v5c0 4.8 2.8 8 7 10 4.2-2 7-5.2 7-10V6l-7-3Zm-3 9 2 2 4-4" />
    <path v-else-if="name === 'devices'" d="M4 5h11v9H4V5Zm3 13h5m-2-4v4m8-9h2a1 1 0 0 1 1 1v9a1 1 0 0 1-1 1h-4a1 1 0 0 1-1-1v-3" />
    <path v-else-if="name === 'route'" d="M5 5h5m4 0h5M5 19h5m4 0h5M8 5c0 7 8 7 8 14M16 5c0 7-8 7-8 14" />
    <path v-else-if="name === 'key'" d="M15 7a5 5 0 1 1-4.2 7.7L3 22v-4l3-3h3l1.8-1.8A5 5 0 0 1 15 7Zm2 3h.01" />
    <path v-else-if="name === 'check'" d="m5 12 4 4L19 6" />
    <path v-else-if="name === 'menu'" d="M4 7h16M4 12h16M4 17h16" />
    <path v-else-if="name === 'close'" d="m6 6 12 12M18 6 6 18" />
    <circle v-else-if="name === 'sun'" cx="12" cy="12" r="4" /><path v-if="name === 'sun'" d="M12 2v2m0 16v2M4.9 4.9l1.4 1.4m11.4 11.4 1.4 1.4M2 12h2m16 0h2M4.9 19.1l1.4-1.4M17.7 6.3l1.4-1.4" />
    <path v-else-if="name === 'moon'" d="M20 15.8A8.5 8.5 0 1 1 8.2 4 7 7 0 0 0 20 15.8Z" />
  </svg>
</template>

<script setup>
defineProps({ name: { type: String, required: true } })
</script>
