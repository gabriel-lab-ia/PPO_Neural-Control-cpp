import * as THREE from "three";

interface EarthTextures {
  day: THREE.Texture;
  night: THREE.Texture;
  clouds: THREE.Texture;
}

type LonLat = [number, number];

interface ContinentShape {
  name: string;
  polygons: LonLat[][];
}

const CONTINENT_SHAPES: ContinentShape[] = [
  {
    name: "north_america",
    polygons: [
      [
        [-168, 72],
        [-155, 66],
        [-145, 60],
        [-132, 54],
        [-125, 47],
        [-123, 40],
        [-118, 33],
        [-110, 28],
        [-97, 24],
        [-89, 20],
        [-82, 24],
        [-78, 31],
        [-70, 42],
        [-65, 49],
        [-75, 58],
        [-96, 66],
        [-120, 71],
        [-145, 73],
      ],
      [
        [-53, 60],
        [-43, 64],
        [-35, 71],
        [-41, 78],
        [-58, 82],
        [-68, 76],
        [-64, 67],
      ],
    ],
  },
  {
    name: "south_america",
    polygons: [
      [
        [-81, 12],
        [-74, 7],
        [-70, 0],
        [-66, -8],
        [-62, -18],
        [-61, -28],
        [-64, -38],
        [-69, -50],
        [-73, -56],
        [-66, -54],
        [-58, -47],
        [-53, -38],
        [-49, -26],
        [-47, -16],
        [-46, -2],
        [-50, 6],
        [-58, 10],
      ],
    ],
  },
  {
    name: "eurasia",
    polygons: [
      [
        [-10, 35],
        [0, 44],
        [16, 52],
        [28, 58],
        [40, 63],
        [56, 66],
        [72, 64],
        [88, 58],
        [104, 52],
        [118, 48],
        [130, 44],
        [140, 48],
        [156, 54],
        [170, 62],
        [172, 52],
        [162, 43],
        [148, 34],
        [132, 26],
        [116, 20],
        [100, 16],
        [82, 10],
        [64, 5],
        [52, 8],
        [42, 16],
        [30, 22],
        [20, 25],
        [12, 30],
      ],
      [
        [66, 25],
        [76, 23],
        [84, 19],
        [86, 10],
        [78, 7],
        [70, 12],
      ],
      [
        [96, 16],
        [106, 12],
        [112, 4],
        [109, -3],
        [100, -4],
        [94, 5],
      ],
      [
        [48, 30],
        [58, 28],
        [62, 24],
        [58, 17],
        [50, 18],
        [46, 23],
      ],
    ],
  },
  {
    name: "africa",
    polygons: [
      [
        [-18, 35],
        [-4, 36],
        [12, 35],
        [24, 31],
        [31, 24],
        [34, 12],
        [39, 2],
        [41, -9],
        [39, -19],
        [31, -29],
        [20, -34],
        [8, -35],
        [-2, -30],
        [-7, -20],
        [-12, -5],
        [-15, 8],
      ],
      [
        [47, -12],
        [50, -17],
        [49, -22],
        [45, -25],
        [42, -20],
      ],
    ],
  },
  {
    name: "australia",
    polygons: [
      [
        [113, -11],
        [122, -15],
        [134, -18],
        [146, -22],
        [153, -29],
        [151, -36],
        [141, -40],
        [131, -38],
        [121, -33],
        [114, -25],
      ],
      [
        [145, -42],
        [148, -44],
        [147, -47],
        [143, -46],
      ],
    ],
  },
  {
    name: "antarctica",
    polygons: [
      [
        [-180, -72],
        [-145, -70],
        [-110, -74],
        [-70, -71],
        [-40, -75],
        [-10, -73],
        [20, -76],
        [50, -72],
        [80, -74],
        [120, -70],
        [150, -73],
        [180, -72],
        [180, -90],
        [-180, -90],
      ],
    ],
  },
];

function clamp01(value: number): number {
  return Math.min(1, Math.max(0, value));
}

function pseudoNoise(x: number, y: number): number {
  const v = Math.sin(x * 12.9898 + y * 78.233) * 43758.5453;
  return v - Math.floor(v);
}

function fractalNoise(x: number, y: number): number {
  let amplitude = 1;
  let frequency = 1;
  let sum = 0;
  let weight = 0;

  for (let octave = 0; octave < 5; octave += 1) {
    sum += pseudoNoise(x * frequency, y * frequency) * amplitude;
    weight += amplitude;
    amplitude *= 0.5;
    frequency *= 2;
  }

  return sum / weight;
}

function createTextureCanvas(width: number, height: number): HTMLCanvasElement {
  const canvas = document.createElement("canvas");
  canvas.width = width;
  canvas.height = height;
  return canvas;
}

function lonLatToUv(lon: number, lat: number): [number, number] {
  const u = (lon + 180) / 360;
  const v = (90 - lat) / 180;
  return [u, v];
}

function drawContinents(context: CanvasRenderingContext2D, width: number, height: number): void {
  context.save();
  context.fillStyle = "#2f6f44";
  context.strokeStyle = "#89b57a";
  context.lineWidth = 1.2;

  for (const continent of CONTINENT_SHAPES) {
    for (const polygon of continent.polygons) {
      context.beginPath();
      polygon.forEach(([lon, lat], index) => {
        const [u, v] = lonLatToUv(lon, lat);
        const x = u * width;
        const y = v * height;
        if (index === 0) {
          context.moveTo(x, y);
        } else {
          context.lineTo(x, y);
        }
      });
      context.closePath();
      context.fill();
      context.stroke();
    }
  }

  context.restore();
}

function addTerrainVariation(context: CanvasRenderingContext2D, width: number, height: number): void {
  const image = context.getImageData(0, 0, width, height);

  for (let y = 0; y < height; y += 1) {
    for (let x = 0; x < width; x += 1) {
      const index = (y * width + x) * 4;
      const r = image.data[index];
      const g = image.data[index + 1];
      const b = image.data[index + 2];

      const u = x / width;
      const v = y / height;
      const n = fractalNoise(u * 8.2, v * 6.1);

      const isLand = g > b;
      const modifier = isLand ? (0.82 + 0.38 * n) : (0.86 + 0.30 * n);
      image.data[index] = Math.floor(clamp01((r / 255) * modifier) * 255);
      image.data[index + 1] = Math.floor(clamp01((g / 255) * modifier) * 255);
      image.data[index + 2] = Math.floor(clamp01((b / 255) * (isLand ? modifier * 0.9 : modifier * 1.2)) * 255);
      image.data[index + 3] = 255;
    }
  }

  context.putImageData(image, 0, 0);
}

function makeDayMap(width: number, height: number): HTMLCanvasElement {
  const canvas = createTextureCanvas(width, height);
  const context = canvas.getContext("2d");
  if (!context) {
    return canvas;
  }

  const gradient = context.createLinearGradient(0, 0, 0, height);
  gradient.addColorStop(0, "#1a3d67");
  gradient.addColorStop(0.5, "#0f3358");
  gradient.addColorStop(1, "#152d4e");
  context.fillStyle = gradient;
  context.fillRect(0, 0, width, height);

  drawContinents(context, width, height);
  addTerrainVariation(context, width, height);

  return canvas;
}

function makeNightMap(width: number, height: number): HTMLCanvasElement {
  const canvas = createTextureCanvas(width, height);
  const context = canvas.getContext("2d");
  if (!context) {
    return canvas;
  }

  context.fillStyle = "#050506";
  context.fillRect(0, 0, width, height);

  drawContinents(context, width, height);

  const image = context.getImageData(0, 0, width, height);
  for (let y = 0; y < height; y += 1) {
    for (let x = 0; x < width; x += 1) {
      const index = (y * width + x) * 4;
      const g = image.data[index + 1];
      const b = image.data[index + 2];
      const isLand = g > b;

      if (!isLand) {
        image.data[index] = 0;
        image.data[index + 1] = 0;
        image.data[index + 2] = 0;
        continue;
      }

      const u = x / width;
      const v = y / height;
      const cityNoise = fractalNoise(u * 24 + 2.3, v * 24 + 4.1);
      const city = clamp01((cityNoise - 0.72) * 3.8);

      image.data[index] = Math.floor(88 + city * 165);
      image.data[index + 1] = Math.floor(54 + city * 122);
      image.data[index + 2] = Math.floor(20 + city * 62);
      image.data[index + 3] = 255;
    }
  }

  context.putImageData(image, 0, 0);
  return canvas;
}

function makeCloudMap(width: number, height: number): HTMLCanvasElement {
  const canvas = createTextureCanvas(width, height);
  const context = canvas.getContext("2d");
  if (!context) {
    return canvas;
  }

  const image = context.createImageData(width, height);
  for (let y = 0; y < height; y += 1) {
    for (let x = 0; x < width; x += 1) {
      const u = x / width;
      const v = y / height;
      const noise = fractalNoise(u * 9.4 + 11.2, v * 7.3 + 2.4);
      const alpha = clamp01((noise - 0.55) * 3.4);

      const index = (y * width + x) * 4;
      image.data[index] = 224;
      image.data[index + 1] = 232;
      image.data[index + 2] = 241;
      image.data[index + 3] = Math.floor(alpha * 172);
    }
  }

  context.putImageData(image, 0, 0);
  return canvas;
}

function toTexture(canvas: HTMLCanvasElement): THREE.Texture {
  const texture = new THREE.CanvasTexture(canvas);
  texture.colorSpace = THREE.SRGBColorSpace;
  texture.wrapS = THREE.RepeatWrapping;
  texture.wrapT = THREE.ClampToEdgeWrapping;
  texture.anisotropy = 8;
  texture.needsUpdate = true;
  return texture;
}

export function createProceduralEarthTextures(): EarthTextures {
  const width = 2048;
  const height = 1024;

  return {
    day: toTexture(makeDayMap(width, height)),
    night: toTexture(makeNightMap(width, height)),
    clouds: toTexture(makeCloudMap(width, height)),
  };
}
