import { defineConfig, defaultTheme } from 'vitepress'

const base = process.env.DOCS_BASE || '/'

const zhSidebar = [
  {
    text: '指南',
    collapsed: false,
    items: [
      { text: '快速开始', link: '/zh/guide/getting-started.html' },
      { text: '架构概览', link: '/zh/guide/architecture.html' },
      { text: '使用指南', link: '/zh/guide/usage.html' },
      { text: '协议规范', link: '/zh/guide/protocol.html' },
    ],
  },
  {
    text: 'API 参考',
    collapsed: false,
    items: [
      { text: 'API 参考', link: '/zh/api/reference.html' },
    ],
  },
  {
    text: '部署',
    collapsed: false,
    items: [
      { text: 'Nacos 集成', link: '/zh/deploy/nacos.html' },
    ],
  },
  {
    text: '其他',
    collapsed: false,
    items: [
      { text: '更新日志', link: '/zh/about/changelog.html' },
      { text: '常见问题', link: '/zh/about/faq.html' },
    ],
  },
]

const enSidebar = [
  {
    text: 'Guide',
    collapsed: false,
    items: [
      { text: 'Getting Started', link: '/en/guide/getting-started.html' },
      { text: 'Architecture', link: '/en/guide/architecture.html' },
      { text: 'Usage', link: '/en/guide/usage.html' },
      { text: 'Protocol', link: '/en/guide/protocol.html' },
    ],
  },
  {
    text: 'API Reference',
    collapsed: false,
    items: [
      { text: 'API Reference', link: '/en/api/reference.html' },
    ],
  },
  {
    text: 'Deploy',
    collapsed: false,
    items: [
      { text: 'Nacos Integration', link: '/en/deploy/nacos.html' },
    ],
  },
  {
    text: 'About',
    collapsed: false,
    items: [
      { text: 'Changelog', link: '/en/about/changelog.html' },
      { text: 'FAQ', link: '/en/about/faq.html' },
    ],
  },
]

export default defineConfig({
  srcDir: '.',
  base,
  lang: 'zh-CN',
  title: 'Mini-RPC',
  description: '轻量级 C++ RPC 框架',
  head: [
    ['link', { rel: 'icon', href: '/logo.svg', type: 'image/svg+xml' }],
    ['meta', { name: 'theme-color', content: '#5f67ee' }],
    ['meta', { property: 'og:type', content: 'website' }],
    ['meta', { property: 'og:locale', content: 'zh-CN' }],
    ['meta', { property: 'og:title', content: 'Mini-RPC' }],
    ['meta', { property: 'og:description', content: '轻量级 C++ RPC 框架' }],
  ],
  themeConfig: {
    logo: { src: '/logo.svg', width: 24, height: 24 },
    outline: { level: [2, 3] },
    socialLinks: [
      { icon: 'github', link: 'https://github.com/desyang-hub/mini-rpc' },
    ],
    footer: {
      message: 'Released under the MIT License.',
      copyright: 'Copyright © 2024-present desyang',
    },
    search: {
      provider: 'local',
      options: {
        detailedView: true,
      },
    },
  },

  locales: {
    root: {
      label: '简体中文',
      lang: 'zh-CN',
      link: '/zh/',
      themeConfig: {
        nav: [
          { text: '指南', link: '/zh/guide/getting-started.html', activeMatch: '^/zh/guide/' },
          { text: 'API', link: '/zh/api/reference.html', activeMatch: '^/zh/api/' },
          { text: '部署', link: '/zh/deploy/nacos.html', activeMatch: '^/zh/deploy/' },
          { text: '关于', link: '/zh/about/changelog.html', activeMatch: '^/zh/about/' },
        ],
        selectLanguageName: '简体中文',
        sidebar: {
          ['^/zh/']: zhSidebar,
        },
        docFooter: { prev: '上一页', next: '下一页' },
        outline: { label: '页面导航' },
        lastUpdated: { text: '更新于' },
        returnToTopLabel: '回到顶部',
        sidebarMenuLabel: '菜单',
        darkModeSwitchLabel: '主题',
        lightModeSwitchTitle: '切换到浅色模式',
        darkModeSwitchTitle: '切换到深色模式',
      },
    },
    en: {
      label: 'English',
      lang: 'en-US',
      link: '/en/',
      themeConfig: {
        nav: [
          { text: 'Guide', link: '/en/guide/getting-started.html', activeMatch: '^/en/guide/' },
          { text: 'API', link: '/en/api/reference.html', activeMatch: '^/en/api/' },
          { text: 'Deploy', link: '/en/deploy/nacos.html', activeMatch: '^/en/deploy/' },
          { text: 'About', link: '/en/about/changelog.html', activeMatch: '^/en/about/' },
        ],
        selectLanguageName: 'English',
        sidebar: {
          ['^/en/']: enSidebar,
        },
        docFooter: { prev: 'Previous', next: 'Next' },
        outline: { label: 'On this page' },
        lastUpdated: { text: 'Last updated' },
        returnToTopLabel: 'Back to top',
        sidebarMenuLabel: 'Menu',
        darkModeSwitchLabel: 'Appearance',
        lightModeSwitchTitle: 'Switch to light mode',
        darkModeSwitchTitle: 'Switch to dark mode',
      },
    },
  },
})
