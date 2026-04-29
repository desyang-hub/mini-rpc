import { writeFileSync } from 'node:fs'

const base = process.env.DOCS_BASE || '/'

const redirectPath = base === '/' ? './zh/' : base + 'zh/'

const redirectHtml = `<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>Mini-RPC</title>
  <meta http-equiv="refresh" content="0;url=${redirectPath}">
  <link rel="canonical" href="${redirectPath}">
</head>
<body>
  <script>window.location.replace('${redirectPath}')</script>
</body>
</html>
`

writeFileSync('.vitepress/dist/index.html', redirectHtml)
console.log(`Created dist/index.html with redirect to ${redirectPath}`)
