/**
 * ModelPeek 3D Previewer Engine
 * High-performance, lightweight WebGL viewer for CAD & 3D models.
 */

class ModelPeekViewer {
    constructor(containerId) {
        this.container = document.getElementById(containerId);
        this.scene = null;
        this.camera = null;
        this.renderer = null;
        this.controls = null;
        this.currentModel = null;
        this.edgeLines = null;
        this.gridHelper = null;
        this.axesHelper = null;
        this.renderMode = 'shaded_edges'; // 'shaded', 'shaded_edges', 'wireframe'
        this.modelBBox = null;
        this.modelCenter = new THREE.Vector3();
        this.modelRadius = 100;

        this.init();
        this.setupEventListeners();
        this.checkUrlParameters();
    }

    init() {
        const width = this.container.clientWidth || window.innerWidth;
        const height = this.container.clientHeight || window.innerHeight;

        // 1. Scene
        this.scene = new THREE.Scene();
        this.scene.background = null; // transparent to allow CSS radial gradient

        // 2. Camera
        this.camera = new THREE.PerspectiveCamera(45, width / height, 0.1, 100000);
        this.camera.position.set(150, 150, 200);

        // 3. Renderer
        this.renderer = new THREE.WebGLRenderer({
            antialias: true,
            alpha: true,
            preserveDrawingBuffer: true,
            powerPreference: "high-performance"
        });
        this.renderer.setSize(width, height);
        this.renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
        this.renderer.toneMapping = THREE.ACESFilmicToneMapping;
        this.renderer.toneMappingExposure = 1.1;
        this.container.appendChild(this.renderer.domElement);

        // 4. OrbitControls
        this.controls = new THREE.OrbitControls(this.camera, this.renderer.domElement);
        this.controls.enableDamping = true;
        this.controls.dampingFactor = 0.08;
        this.controls.screenSpacePanning = true;
        this.controls.maxDistance = 50000;
        this.controls.minDistance = 1;

        // 5. Lighting Setup (Professional CAD Studio Lights)
        const ambientLight = new THREE.AmbientLight(0xffffff, 0.65);
        this.scene.add(ambientLight);

        const keyLight = new THREE.DirectionalLight(0xffffff, 0.75);
        keyLight.position.set(1, 1.5, 1).normalize();
        this.scene.add(keyLight);

        const fillLight = new THREE.DirectionalLight(0xd8e2ec, 0.45);
        fillLight.position.set(-1, 0.8, -1).normalize();
        this.scene.add(fillLight);

        const backLight = new THREE.DirectionalLight(0x88c0d0, 0.35);
        backLight.position.set(0, -1, 0).normalize();
        this.scene.add(backLight);

        // 6. Helpers
        this.createEnvironmentHelpers(200);

        // 7. Animation Loop
        const animate = () => {
            requestAnimationFrame(animate);
            this.controls.update();
            this.renderer.render(this.scene, this.camera);
        };
        animate();

        window.addEventListener('resize', () => this.onWindowResize());
    }

    createEnvironmentHelpers(size) {
        if (this.gridHelper) this.scene.remove(this.gridHelper);
        if (this.axesHelper) this.scene.remove(this.axesHelper);

        // Subtle CAD Ground Grid
        this.gridHelper = new THREE.GridHelper(size * 2, 20, 0x4c566a, 0x3b4252);
        this.gridHelper.position.y = 0;
        this.scene.add(this.gridHelper);

        // RGB Coordinate axes helper
        this.axesHelper = new THREE.AxesHelper(size * 0.4);
        this.axesHelper.renderOrder = 1;
        this.scene.add(this.axesHelper);
    }

    onWindowResize() {
        const width = this.container.clientWidth || window.innerWidth;
        const height = this.container.clientHeight || window.innerHeight;
        this.camera.aspect = width / height;
        this.camera.updateProjectionMatrix();
        this.renderer.setSize(width, height);
    }

    // Load 3D model from URL / local file path
    loadModel(filePath) {
        this.showLoader(`加载中: ${filePath.split(/[\/\\]/).pop()}...`);
        const ext = filePath.split('.').pop().toLowerCase();

        const onProgress = (xhr) => {
            if (xhr.lengthComputable) {
                const percent = Math.round((xhr.loaded / xhr.total) * 100);
                this.updateLoaderText(`加载中: ${percent}%`);
            }
        };

        const onError = (error) => {
            console.error("加载模型失败:", error);
            this.hideLoader();
            this.showToast("模型加载失败，请检查文件格式或损坏状态");
        };

        try {
            switch (ext) {
                case 'stl':
                    new THREE.STLLoader().load(filePath, (geometry) => {
                        const material = this.createDefaultMaterial();
                        const mesh = new THREE.Mesh(geometry, material);
                        this.setModel(mesh, filePath);
                    }, onProgress, onError);
                    break;

                case 'obj':
                    new THREE.OBJLoader().load(filePath, (object) => {
                        this.setModel(object, filePath);
                    }, onProgress, onError);
                    break;

                case 'glb':
                case 'gltf':
                    new THREE.GLTFLoader().load(filePath, (gltf) => {
                        this.setModel(gltf.scene, filePath);
                    }, onProgress, onError);
                    break;

                case '3mf':
                    new THREE.ThreeMFLoader().load(filePath, (object) => {
                        this.setModel(object, filePath);
                    }, onProgress, onError);
                    break;

                default:
                    this.hideLoader();
                    this.showToast(`暂不支持直接在视口解析 .${ext}，尝试由 Worker 转换后预览`);
                    break;
            }
        } catch (err) {
            onError(err);
        }
    }

    // Set model into scene and calculate stats
    setModel(object, filePath) {
        if (this.currentModel) {
            this.scene.remove(this.currentModel);
            this.currentModel = null;
        }

        this.currentModel = object;
        this.scene.add(this.currentModel);

        // Normalize materials & gather statistics
        let vertexCount = 0;
        let faceCount = 0;

        this.currentModel.traverse((child) => {
            if (child.isMesh) {
                if (!child.material || child.material.type === 'MeshBasicMaterial') {
                    child.material = this.createDefaultMaterial();
                } else {
                    child.material.side = THREE.DoubleSide;
                }

                if (child.geometry) {
                    if (child.geometry.attributes && child.geometry.attributes.position) {
                        vertexCount += child.geometry.attributes.position.count;
                    }
                    if (child.geometry.index) {
                        faceCount += child.geometry.index.count / 3;
                    } else if (child.geometry.attributes && child.geometry.attributes.position) {
                        faceCount += child.geometry.attributes.position.count / 3;
                    }
                }
            }
        });

        // Compute Bounding Box
        const bbox = new THREE.Box3().setFromObject(this.currentModel);
        this.modelBBox = bbox;
        const size = new THREE.Vector3();
        bbox.getSize(size);
        bbox.getCenter(this.modelCenter);
        this.modelRadius = Math.max(size.x, size.y, size.z) || 100;

        // Position ground grid just below the model
        this.createEnvironmentHelpers(this.modelRadius);
        if (this.gridHelper) {
            this.gridHelper.position.y = bbox.min.y - (size.y * 0.01);
        }

        // Generate Edge Lines for CAD look
        this.generateEdgeLines();

        // Fit Camera View
        this.fitView();

        // Update UI Panel
        const fileName = filePath ? filePath.split(/[\/\\]/).pop() : "3D Model";
        this.updateInfoPanel({
            name: fileName,
            sizeX: size.x.toFixed(2),
            sizeY: size.y.toFixed(2),
            sizeZ: size.z.toFixed(2),
            vertices: Math.round(vertexCount).toLocaleString(),
            faces: Math.round(faceCount).toLocaleString()
        });

        this.hideLoader();
    }

    createDefaultMaterial() {
        return new THREE.MeshStandardMaterial({
            color: 0x90a4ae,
            metalness: 0.25,
            roughness: 0.45,
            side: THREE.DoubleSide
        });
    }

    generateEdgeLines() {
        if (this.edgeLines) {
            this.scene.remove(this.edgeLines);
            this.edgeLines = null;
        }

        const edgeGroup = new THREE.Group();
        this.currentModel.traverse((child) => {
            if (child.isMesh && child.geometry) {
                const edgesGeometry = new THREE.EdgesGeometry(child.geometry, 28);
                const line = new THREE.LineSegments(
                    edgesGeometry,
                    new THREE.LineBasicMaterial({ color: 0x2e3440, linewidth: 1 })
                );
                line.matrix = child.matrixWorld;
                line.matrixAutoUpdate = false;
                edgeGroup.add(line);
            }
        });

        this.edgeLines = edgeGroup;
        if (this.renderMode === 'shaded_edges') {
            this.scene.add(this.edgeLines);
        }
    }

    // Set Render Mode: 'shaded', 'shaded_edges', 'wireframe'
    setRenderMode(mode) {
        this.renderMode = mode;
        if (!this.currentModel) return;

        if (this.edgeLines) {
            if (mode === 'shaded_edges') {
                this.scene.add(this.edgeLines);
            } else {
                this.scene.remove(this.edgeLines);
            }
        }

        this.currentModel.traverse((child) => {
            if (child.isMesh && child.material) {
                child.material.wireframe = (mode === 'wireframe');
            }
        });
    }

    // Preset Views
    setView(viewType) {
        if (!this.modelBBox) return;

        const center = this.modelCenter;
        const dist = this.modelRadius * 2.2;

        switch (viewType) {
            case 'iso':
                this.camera.position.set(center.x + dist, center.y + dist * 0.8, center.z + dist);
                break;
            case 'top':
                this.camera.position.set(center.x, center.y + dist * 1.5, center.z + 0.0001);
                break;
            case 'front':
                this.camera.position.set(center.x, center.y, center.z + dist * 1.5);
                break;
            case 'right':
                this.camera.position.set(center.x + dist * 1.5, center.y, center.z);
                break;
        }

        this.controls.target.copy(center);
        this.controls.update();
    }

    fitView() {
        if (!this.modelBBox) return;

        const center = this.modelCenter;
        const maxDim = this.modelRadius;
        const fov = this.camera.fov * (Math.PI / 180);
        let cameraZ = Math.abs(maxDim / 2 / Math.tan(fov / 2)) * 2.4;

        this.camera.position.set(center.x + cameraZ * 0.7, center.y + cameraZ * 0.6, center.z + cameraZ * 0.7);
        this.camera.lookAt(center);
        this.controls.target.copy(center);
        this.controls.update();
    }

    setupEventListeners() {
        // Render mode buttons
        document.querySelectorAll('[data-render-mode]').forEach(btn => {
            btn.addEventListener('click', (e) => {
                document.querySelectorAll('[data-render-mode]').forEach(b => b.classList.remove('active'));
                btn.classList.add('active');
                this.setRenderMode(btn.dataset.renderMode);
            });
        });

        // View preset buttons
        document.querySelectorAll('[data-view]').forEach(btn => {
            btn.addEventListener('click', () => {
                this.setView(btn.dataset.view);
            });
        });

        // Fit view button
        const fitBtn = document.getElementById('btn-fit');
        if (fitBtn) {
            fitBtn.addEventListener('click', () => this.fitView());
        }

        // Drag & Drop local file support
        window.addEventListener('dragover', (e) => e.preventDefault());
        window.addEventListener('drop', (e) => {
            e.preventDefault();
            if (e.dataTransfer.files.length > 0) {
                const file = e.dataTransfer.files[0];
                const url = URL.createObjectURL(file);
                this.loadModel(url, file.name);
            }
        });

        // WebView2 host message listener
        if (window.chrome && window.chrome.webview) {
            window.chrome.webview.addEventListener('message', (event) => {
                const data = event.data;
                if (typeof data === 'string') {
                    this.loadModel(data);
                } else if (data && data.action === 'load') {
                    this.loadModel(data.path);
                }
            });
        }
    }

    checkUrlParameters() {
        const params = new URLSearchParams(window.location.search);
        const file = params.get('file');
        if (file) {
            this.loadModel(file);
        } else {
            // Display empty welcome guide
            this.hideLoader();
            this.updateInfoPanel({
                name: "ModelPeek 就绪",
                sizeX: "--",
                sizeY: "--",
                sizeZ: "--",
                vertices: "--",
                faces: "--"
            });
        }
    }

    showLoader(text) {
        const overlay = document.getElementById('loader-overlay');
        const label = document.getElementById('loader-text');
        if (overlay) overlay.style.display = 'flex';
        if (label) label.innerText = text;
    }

    updateLoaderText(text) {
        const label = document.getElementById('loader-text');
        if (label) label.innerText = text;
    }

    hideLoader() {
        const overlay = document.getElementById('loader-overlay');
        if (overlay) overlay.style.display = 'none';
    }

    showToast(msg) {
        const toast = document.getElementById('toast');
        if (toast) {
            toast.innerText = msg;
            toast.style.display = 'block';
            setTimeout(() => { toast.style.display = 'none'; }, 3500);
        }
    }

    updateInfoPanel(stats) {
        document.getElementById('stat-name').innerText = stats.name;
        document.getElementById('stat-dims').innerText = `${stats.sizeX} × ${stats.sizeY} × ${stats.sizeZ} mm`;
        document.getElementById('stat-vertices').innerText = stats.vertices;
        document.getElementById('stat-faces').innerText = stats.faces;
    }
}

// Instantiate on page load
window.addEventListener('DOMContentLoaded', () => {
    window.viewer = new ModelPeekViewer('canvas-container');
});
