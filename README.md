# procon26 mvp

このリポジトリは、M5Stack を使った BLE 連携の試作プロジェクトです。`ble_send` と `m5` はどちらもプロトタイプであり、実装や仕様は今後変更される前提です。

## プロジェクト構成

### `ble_send`

M5Stack Core から BLE Beacon を送信する送信側プロトタイプです。`M5_BEACON` という名前で広告を出し、受信側が RSSI を使って距離の目安を取得できるようにしています。

- 対象ボード: `M5Stack Core`
- PlatformIO 環境: `m5stack-core-esp32`
- 主な役割: BLE Beacon の広告送信
- 表示: 起動状態を簡易表示

### `m5`

M5Stack CoreS3 で動作する受信側プロトタイプです。BLE Beacon をスキャンして RSSI を取得し、歩数とあわせて SD カードへ CSV 保存します。

- 対象ボード: `M5Stack CoreS3`
- PlatformIO 環境: `m5stack-cores3`
- 主な役割: BLE 受信、歩数計測、CSV ログ保存
- 補足: `M5Unified` を使用しています

## 動作イメージ

1. `ble_send` を起動して `M5_BEACON` を送信する
2. `m5` を起動して Beacon をスキャンする
3. `m5` 側で RSSI と歩数を取得し、SD カードに記録する

## 注意事項

- どちらも試作段階のため、動作保証や安定性は最終版ではありません。
- ハードウェアやファームウェアの変更に合わせて、ボード設定やライブラリ構成が変わる可能性があります。
- `m5` 側は SD カードを使うため、実機での確認が前提です。

## ビルドメモ

- `ble_send`: PlatformIO で `m5stack-core-esp32` 環境を利用する
- `m5`: PlatformIO で `m5stack-cores3` 環境を利用する
