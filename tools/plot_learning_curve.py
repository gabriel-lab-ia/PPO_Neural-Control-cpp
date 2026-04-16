#!/usr/bin/env python3
from __future__ import annotations

import csv
import math
import os
import sys
from typing import Dict, List, Tuple


def read_rows(path: str) -> List[Dict[str, float | str]]:
    rows: List[Dict[str, float | str]] = []
    with open(path, "r", encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            parsed: Dict[str, float | str] = {}
            for key, value in row.items():
                if key is None or value in (None, ""):
                    continue
                parsed[key] = float(value)
            rows.append(parsed)
    return rows


def scale_points(
    points: List[Tuple[float, float]],
    x_min: float,
    x_max: float,
    y_min: float,
    y_max: float,
    left: float,
    top: float,
    width: float,
    height: float,
) -> str:
    if not points:
        return ""

    def sx(x: float) -> float:
        if math.isclose(x_max, x_min):
            return left + width / 2.0
        return left + ((x - x_min) / (x_max - x_min)) * width

    def sy(y: float) -> float:
        if math.isclose(y_max, y_min):
            return top + height / 2.0
        return top + height - ((y - y_min) / (y_max - y_min)) * height

    return " ".join(f"{sx(x):.2f},{sy(y):.2f}" for x, y in points)


def draw_axes(svg: List[str], left: float, top: float, width: float, height: float, title: str, y_min: float, y_max: float) -> None:
    svg.append(f'<rect x="{left:.2f}" y="{top:.2f}" width="{width:.2f}" height="{height:.2f}" fill="#101924" stroke="#314154" stroke-width="1" rx="18"/>')
    svg.append(f'<text x="{left:.2f}" y="{top - 14:.2f}" fill="#dce7f3" font-size="18" font-family="DejaVu Sans, sans-serif">{title}</text>')

    for tick in range(5):
        y = top + (height * tick / 4.0)
        value = y_max - ((y_max - y_min) * tick / 4.0)
        svg.append(f'<line x1="{left:.2f}" y1="{y:.2f}" x2="{left + width:.2f}" y2="{y:.2f}" stroke="#223041" stroke-width="1"/>')
        svg.append(f'<text x="{left - 10:.2f}" y="{y + 4:.2f}" text-anchor="end" fill="#93a8bf" font-size="11" font-family="DejaVu Sans, sans-serif">{value:.2f}</text>')


def add_series(
    svg: List[str],
    rows: List[Dict[str, float | str]],
    x_key: str,
    left: float,
    top: float,
    width: float,
    height: float,
    title: str,
    metrics: List[Tuple[str, str, str]],
) -> None:
    if not rows:
        return

    x_values = [float(row[x_key]) for row in rows]
    all_values = [float(row[key]) for row in rows for key, _, _ in metrics]
    x_min, x_max = min(x_values), max(x_values)
    y_min, y_max = min(all_values), max(all_values)
    margin = max(0.05 * (y_max - y_min), 0.05)
    y_min -= margin
    y_max += margin

    draw_axes(svg, left, top, width, height, title, y_min, y_max)

    for key, label, color in metrics:
        points = [(float(row[x_key]), float(row[key])) for row in rows]
        polyline = scale_points(points, x_min, x_max, y_min, y_max, left, top, width, height)
        svg.append(f'<polyline fill="none" stroke="{color}" stroke-width="2.5" points="{polyline}"/>')

    for tick in range(5):
        x = left + (width * tick / 4.0)
        value = x_min + ((x_max - x_min) * tick / 4.0)
        svg.append(f'<line x1="{x:.2f}" y1="{top:.2f}" x2="{x:.2f}" y2="{top + height:.2f}" stroke="#1a2633" stroke-width="1"/>')
        svg.append(f'<text x="{x:.2f}" y="{top + height + 18:.2f}" text-anchor="middle" fill="#93a8bf" font-size="11" font-family="DejaVu Sans, sans-serif">{value:.0f}</text>')

    legend_x = left + 12
    legend_y = top + 18
    for index, (_, label, color) in enumerate(metrics):
        offset_y = legend_y + index * 18
        svg.append(f'<line x1="{legend_x:.2f}" y1="{offset_y:.2f}" x2="{legend_x + 18:.2f}" y2="{offset_y:.2f}" stroke="{color}" stroke-width="3"/>')
        svg.append(f'<text x="{legend_x + 24:.2f}" y="{offset_y + 4:.2f}" fill="#dce7f3" font-size="12" font-family="DejaVu Sans, sans-serif">{label}</text>')


def add_stat_card(svg: List[str], x: float, y: float, width: float, title: str, value: str) -> None:
    svg.append(f'<rect x="{x:.2f}" y="{y:.2f}" width="{width:.2f}" height="82.00" rx="18" fill="#101924" stroke="#314154" stroke-width="1"/>')
    svg.append(f'<text x="{x + 18:.2f}" y="{y + 28:.2f}" fill="#8ea6bd" font-size="12" font-family="DejaVu Sans, sans-serif" letter-spacing="1.2">{title}</text>')
    svg.append(f'<text x="{x + 18:.2f}" y="{y + 58:.2f}" fill="#f4f8fc" font-size="24" font-family="DejaVu Sans, sans-serif">{value}</text>')


def render_svg(rows: List[Dict[str, float | str]], output_path: str) -> None:
    width = 1200
    height = 1160
    svg: List[str] = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
        '<rect width="100%" height="100%" fill="#08111b"/>',
        '<text x="60" y="50" fill="#f4f8fc" font-size="28" font-family="DejaVu Sans, sans-serif">Orbital Neural Control CPP - PPO Learning Curve</text>',
        '<text x="60" y="80" fill="#9cb3ca" font-size="14" font-family="DejaVu Sans, sans-serif">Learning curve generated from training_metrics.csv with stability and efficiency diagnostics.</text>',
    ]

    final = rows[-1]
    card_y = 110
    card_width = 196
    card_gap = 16
    stats = [
        ("Final Value Loss", f"{float(final['value_loss']):.3f}"),
        ("Final Entropy", f"{float(final['entropy']):.3f}"),
        ("Action Std", f"{float(final['action_std']):.3f}"),
        ("Latency", f"{float(final['inference_latency_ms']):.3f} ms"),
        ("Throughput", f"{float(final['samples_per_second']):.0f} sps"),
    ]
    for index, (title, value) in enumerate(stats):
        add_stat_card(svg, 60 + index * (card_width + card_gap), card_y, card_width, title, value)

    chart_left = 60
    chart_width = 1080
    chart_height = 250

    add_series(
        svg,
        rows,
        "update",
        chart_left,
        250,
        chart_width,
        chart_height,
        "Optimization diagnostics",
        [
            ("policy_loss", "policy loss", "#4fc3f7"),
            ("value_loss", "value loss", "#ffca28"),
            ("entropy", "entropy", "#66bb6a"),
            ("approx_kl", "approx kl", "#ef5350"),
        ],
    )

    add_series(
        svg,
        rows,
        "update",
        chart_left,
        590,
        chart_width,
        chart_height,
        "Control quality",
        [
            ("avg_episode_return", "avg episode return", "#42a5f5"),
            ("success_rate", "success rate", "#ab47bc"),
            ("action_std", "action std", "#26a69a"),
            ("explained_variance", "explained variance", "#ffa726"),
        ],
    )

    add_series(
        svg,
        rows,
        "update",
        chart_left,
        930,
        chart_width,
        chart_height,
        "Efficiency diagnostics",
        [
            ("update_time_ms", "update time ms", "#29b6f6"),
            ("samples_per_second", "samples per second", "#8bc34a"),
            ("inference_latency_ms", "inference latency ms", "#ff7043"),
        ],
    )

    svg.append('</svg>')
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, 'w', encoding='utf-8') as handle:
        handle.write('\n'.join(svg))


def main() -> int:
    if len(sys.argv) != 3:
        print('usage: plot_learning_curve.py <input.csv> <output.svg>', file=sys.stderr)
        return 1

    rows = read_rows(sys.argv[1])
    if not rows:
        print('csv has no rows', file=sys.stderr)
        return 1

    render_svg(rows, sys.argv[2])
    print(sys.argv[2])
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
