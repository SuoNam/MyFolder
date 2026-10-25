"""Optional Playwright test: build and preview on port 4179 first.
All API requests use fixtures, never real accounts or files.
"""
import argparse
import json
import re
from pathlib import Path
from urllib.parse import urlsplit
from playwright.sync_api import expect, sync_playwright

ROOT = Path(__file__).resolve().parents[1]
LONG_NAME = '跨平台交付测试_' + 'long-filename-' * 12 + '.pdf'
STAMP = '2026-10-25T08:00:00Z'
WEB = dict(deviceId='web-fixture', userId='tester', deviceName='移动浏览器', deviceType='WEB', os='Android',
           deviceAddress=None, listenPort=None, clientVersion='1.1.2', createdAt=STAMP, lastSeenAt=STAMP, online=True)
PEER = dict(WEB, deviceId='peer-fixture', deviceName='测试电脑_' + 'hostname-' * 10, deviceType='PC', os='Ubuntu Linux')
GROUP = dict(groupId='group-fixture', name='共享项目_' + 'project-' * 15, ownerAccount='tester', myPermission='OWNER',
             createdAt=STAMP, members=[dict(account='tester', displayName='测试管理员', permission='OWNER', joinedAt=STAMP),
             dict(account='member-' * 15, displayName='跨设备协作成员', permission='WRITE', joinedAt=STAMP)])
FORWARD = dict(forwardId='forward-fixture', userId='tester', sourceDeviceId='web-fixture', targetDeviceId='peer-fixture',
               destinationPath='MyFolder/' + 'shared-folder/' * 10, deleteSource=False, channel='RELAY', relayUploadId=None,
               files=[dict(path=LONG_NAME, size=123456, sha256='a' * 64)], totalBytes=123456, transferredBytes=123456,
               state='COMPLETED', createdAt=STAMP, updatedAt=STAMP, failureReason=None)


def fixture(route):
    endpoint = urlsplit(route.request.url).path
    if endpoint == '/api/v1/devices':
        body = dict(device=WEB, deviceToken='fixture-token') if route.request.method == 'POST' else [WEB, PEER]
    elif endpoint.endswith('/heartbeat'):
        body = {}
    elif endpoint == '/api/v1/forwards':
        body = [dict(FORWARD, state='FAILED', failureReason='NETWORK_TIMEOUT')]
    elif endpoint == '/api/v1/forwards/history':
        body = [FORWARD]
    elif endpoint == '/api/v1/transfers/tasks':
        body = []
    elif endpoint == '/api/v1/updates/desktop':
        body = dict(platform='linux' if 'platform=linux' in route.request.url else 'windows', latestVersion='1.1.1',
                    withdrawn=False, size=0, sha256='', downloadUrl='')
    elif endpoint == '/api/v1/groups':
        body = dict(status=200, message='ok', data=[GROUP])
    elif endpoint == '/file/getfilelist':
        body = dict(status=200, message='ok', data=[dict(type='File', name=LONG_NAME, path=LONG_NAME, size=123456, lastModified=STAMP),
                    dict(type='Directory', name='共享资料', path='共享资料', lastModified=STAMP)])
    elif endpoint == '/user/me':
        body = dict(status=200, message='ok', data=dict(account='tester', displayName='测试管理员', email='test@example.test',
                    emailVerified=True, autoAcceptDeviceTransfers=False, oauthBindings=[]))
    elif endpoint == '/user/me/storage':
        body = dict(status=200, message='ok', data=dict(tier='FREE', systemRole='USER', usedBytes=123456, limitBytes=1073741824, availableBytes=1073618368))
    elif endpoint == '/user/oauth/providers':
        body = dict(status=200, message='ok', data=[dict(provider=name, configured=True) for name in ['nyauth', 'google', 'github']])
    else:
        raise AssertionError(f'Unexpected API request: {route.request.method} {endpoint}')
    route.fulfill(status=200, content_type='application/json', body=json.dumps(body, ensure_ascii=False))


def assert_fits(page, description):
    size = page.evaluate('''() => ({ viewport: innerWidth, document: document.documentElement.scrollWidth,
      offenders: [...document.querySelectorAll('*')].filter(e => e.getBoundingClientRect().right > innerWidth + 1)
        .slice(0, 10).map(e => e.className) })''')
    assert size['document'] <= size['viewport'] + 1, (description, size)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--url', default='http://127.0.0.1:4179')
    parser.add_argument('--chrome', default=r'C:\Users\Lenovo\AppData\Local\ms-playwright\chromium-1208\chrome-win64\chrome.exe')
    args = parser.parse_args()
    artifacts = ROOT / '.test-artifacts' / 'responsive'
    artifacts.mkdir(parents=True, exist_ok=True)
    with sync_playwright() as pw:
        browser = pw.chromium.launch(executable_path=args.chrome, headless=True)
        for width, height in [(320, 740), (375, 812), (390, 844), (667, 375), (768, 1024), (1280, 800)]:
            mobile = width <= 760
            context = browser.new_context(viewport=dict(width=width, height=height), has_touch=mobile, is_mobile=mobile, locale='zh-CN')
            context.route('https://api.myfolder.com.cn/**', fixture)
            context.add_init_script('''localStorage.setItem('user-token', 'fixture-jwt'); localStorage.setItem('user-account', 'tester');
              localStorage.setItem('mf-device-id', 'web-fixture'); localStorage.setItem('mf-device-token', 'fixture-token');
              localStorage.setItem('mf-device-account', 'tester');''')
            page = context.new_page()
            errors = []
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.goto(args.url + '/files')
            expect(page.locator('.file-name').filter(has_text=LONG_NAME)).to_be_visible()
            assert_fits(page, f'files-{width}')
            if mobile:
                assert page.locator('.file-browser .col-act').first.bounding_box()['width'] > 100
            page.get_by_role('checkbox', name=f'选择 {LONG_NAME}', exact=True).check()
            assert_fits(page, f'bulk-actions-{width}')
            page.get_by_role('button', name='移动', exact=True).click()
            expect(page.get_by_role('heading', name='移动 1 项到')).to_be_visible()
            assert_fits(page, f'move-dialog-{width}')
            page.get_by_role('button', name='取消', exact=True).click()
            page.get_by_role('button', name='上传', exact=True).click()
            expect(page.get_by_role('dialog', name='新建传输')).to_be_visible()
            assert_fits(page, f'upload-dialog-{width}')
            page.get_by_role('button', name='取消', exact=True).click()
            page.evaluate('scrollTo(0, 0)')
            page.screenshot(path=str(artifacts / f'files-{width}.png'), full_page=True)
            for text, title in [('群组', '共享群组'), ('设备', '设备'), ('传输', '传输'), ('传输记录', '传输记录'), ('设置', '设置')]:
                if mobile:
                    page.get_by_role('button', name='打开导航菜单').click()
                page.locator('.nav-item').filter(has=page.locator('span', has_text=re.compile('^' + text + '$'))).click()
                expect(page.get_by_role('heading', name=title, exact=True)).to_be_visible()
                if mobile:
                    expect(page.locator('#main-navigation')).to_be_hidden()
                if text == '群组':
                    expect(page.get_by_text(GROUP['name'], exact=True)).to_be_visible()
                    page.get_by_role('button', name='新建群组').click()
                if text == '传输记录':
                    expect(page.locator('.history-table')).to_be_visible()
                if text == '设置':
                    expect(page.get_by_text('test@example.test', exact=True)).to_be_visible()
                assert_fits(page, f'{text}-{width}')
                page.evaluate('scrollTo(0, 0)')
                page.screenshot(path=str(artifacts / f'{text}-{width}.png'), full_page=True)
            if mobile:
                toggle = page.get_by_role('button', name='打开导航菜单')
                toggle.click()
                close = page.get_by_role('button', name='关闭导航菜单')
                expect(close).to_be_focused()
                close.press('Shift+Tab')
                expect(page.locator('.nav-item').last).to_be_focused()
                page.keyboard.press('Escape')
                expect(toggle).to_be_focused()
            page.get_by_role('button', name='退出登录', exact=True).click()
            expect(page.locator('#acct')).to_be_visible()
            assert_fits(page, f'login-{width}')
            page.get_by_role('button', name='忘记密码？').click()
            expect(page.locator('#code')).to_be_visible()
            assert_fits(page, f'forgot-{width}')
            page.get_by_role('button', name='返回登录').click()
            page.get_by_role('button', name='创建账号').click()
            expect(page.locator('#pwd2')).to_be_visible()
            assert_fits(page, f'signup-{width}')
            assert not errors, errors
            print(f'PASS {width}x{height}', flush=True)
            context.close()
        browser.close()


if __name__ == '__main__':
    main()
