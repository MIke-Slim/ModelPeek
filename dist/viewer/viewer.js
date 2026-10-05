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

        // Measurement & Section Tools
        this.isMeasuring = false;
        this.measurePoints = [];
        this.measureMarkers = [];
        this.measureLine = null;
        this.raycaster = new THREE.Raycaster();
        this.mouse = new THREE.Vector2();

        this.isSectioning = false;
        this.sectionAxis = 'x';
        this.sectionInvert = false;
        this.sectionSliderVal = 50;
        this.clipPlane = new THREE.Plane(new THREE.Vector3(1, 0, 0), 0);

        // Assembly Model Tree & Themes
        this.modelComponents = [];
        this.selectedComponentId = null;
        this.highlightHelper = null;
        this.treeSearchFilter = '';
        this.currentTheme = 'theme-dark';
        this.ambientLight = null;

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
        this.renderer.localClippingEnabled = true;
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
        this.ambientLight = ambientLight;
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
        let fileUrl = filePath;
        if (!fileUrl.startsWith('file://') && !fileUrl.startsWith('http://') && !fileUrl.startsWith('https://')) {
            fileUrl = fileUrl.replace(/\\/g, '/');
            if (/^[a-zA-Z]:/.test(fileUrl)) {
                fileUrl = 'file:///' + fileUrl;
            }
        }

        const fileName = filePath.split(/[\/\\]/).pop();
        this.showLoader(`加载中: ${fileName}...`);
        const ext = fileName.split('.').pop().toLowerCase();

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
                    new THREE.STLLoader().load(fileUrl, (geometry) => {
                        const material = this.createDefaultMaterial();
                        const mesh = new THREE.Mesh(geometry, material);
                        this.setModel(mesh, filePath);
                    }, onProgress, onError);
                    break;

                case 'obj':
                    new THREE.OBJLoader().load(fileUrl, (object) => {
                        this.setModel(object, filePath);
                    }, onProgress, onError);
                    break;

                case 'glb':
                case 'gltf':
                    new THREE.GLTFLoader().load(fileUrl, (gltf) => {
                        this.setModel(gltf.scene, filePath);
                    }, onProgress, onError);
                    break;

                case '3mf':
                    new THREE.ThreeMFLoader().load(fileUrl, (object) => {
                        this.setModel(object, filePath);
                    }, onProgress, onError);
                    break;

                case 'fbx':
                    new THREE.FBXLoader().load(fileUrl, (object) => {
                        this.setModel(object, filePath);
                    }, onProgress, onError);
                    break;

                case 'ply':
                    new THREE.PLYLoader().load(fileUrl, (geometry) => {
                        let material;
                        if (geometry.hasAttribute('color')) {
                            material = new THREE.MeshStandardMaterial({ vertexColors: true, roughness: 0.45, metalness: 0.2, side: THREE.DoubleSide });
                        } else {
                            material = this.createDefaultMaterial();
                        }
                        const mesh = new THREE.Mesh(geometry, material);
                        this.setModel(mesh, filePath);
                    }, onProgress, onError);
                    break;

                case 'gcode':
                    new THREE.GCodeLoader().load(fileUrl, (object) => {
                        this.setModel(object, filePath);
                    }, onProgress, onError);
                    break;

                case 'dae':
                    new THREE.ColladaLoader().load(fileUrl, (collada) => {
                        this.setModel(collada.scene, filePath);
                    }, onProgress, onError);
                    break;

                case '3ds':
                    new THREE.TDSLoader().load(fileUrl, (object) => {
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
            } else if (child.isLine || child.isLineSegments) {
                if (child.material) {
                    if (child.material.color && (child.material.color.getHex() === 0x3300ff || child.material.color.getHex() === 0x000000)) {
                        child.material.color.setHex(0x00d2ff);
                    }
                    if (child.material.linewidth !== undefined) {
                        child.material.linewidth = 2;
                    }
                } else {
                    child.material = new THREE.LineBasicMaterial({ color: 0x00d2ff, linewidth: 2 });
                }
                if (child.geometry && child.geometry.attributes && child.geometry.attributes.position) {
                    vertexCount += child.geometry.attributes.position.count;
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

        // Collect model components for Assembly Tree
        this.modelComponents = [];
        this.selectedComponentId = null;
        this.clearHighlight();
        let partIndex = 1;
        this.currentModel.traverse((child) => {
            if (child.isMesh || child.isLine || child.isLineSegments) {
                let pFaces = 0;
                let pVerts = 0;
                if (child.geometry) {
                    if (child.geometry.attributes && child.geometry.attributes.position) {
                        pVerts = child.geometry.attributes.position.count;
                    }
                    if (child.geometry.index) {
                        pFaces = child.geometry.index.count / 3;
                    } else if (child.geometry.attributes && child.geometry.attributes.position && child.isMesh) {
                        pFaces = child.geometry.attributes.position.count / 3;
                    }
                }
                const rawName = (child.name && child.name.trim() !== '') ? child.name : `部件 #${partIndex++}`;
                const comp = {
                    id: this.modelComponents.length,
                    name: rawName,
                    object: child,
                    faceCount: Math.round(pFaces),
                    vertexCount: Math.round(pVerts),
                    type: child.isMesh ? 'mesh' : 'line',
                    visible: true
                };
                child.userData.componentId = comp.id;
                this.modelComponents.push(comp);
            }
        });
        this.buildModelTreeUI();

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
                line.userData.sourceObject = child;
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

        // Measurement button
        const measureBtn = document.getElementById('btn-measure');
        if (measureBtn) {
            measureBtn.addEventListener('click', () => this.toggleMeasure());
        }
        const measureClearBtn = document.getElementById('btn-measure-clear');
        if (measureClearBtn) {
            measureClearBtn.addEventListener('click', () => this.clearMeasure());
        }
        const measureCloseBtn = document.getElementById('btn-measure-close');
        if (measureCloseBtn) {
            measureCloseBtn.addEventListener('click', () => this.toggleMeasure(false));
        }

        // Section button & controls
        const sectionBtn = document.getElementById('btn-section');
        if (sectionBtn) {
            sectionBtn.addEventListener('click', () => this.toggleSection());
        }
        document.querySelectorAll('#section-bar [data-axis]').forEach(b => {
            b.addEventListener('click', () => this.setSectionAxis(b.dataset.axis));
        });
        const secSlider = document.getElementById('sec-slider');
        if (secSlider) {
            secSlider.addEventListener('input', (e) => {
                this.sectionSliderVal = parseFloat(e.target.value);
                this.updateSectionPlane();
            });
        }
        const secInvert = document.getElementById('sec-invert');
        if (secInvert) {
            secInvert.addEventListener('click', () => {
                this.sectionInvert = !this.sectionInvert;
                secInvert.classList.toggle('active', this.sectionInvert);
                this.updateSectionPlane();
            });
        }
        const secCloseBtn = document.getElementById('btn-section-close');
        if (secCloseBtn) {
            secCloseBtn.addEventListener('click', () => this.toggleSection(false));
        }

        // Model Tree Drawer toggle & actions
        const treeBtn = document.getElementById('btn-tree');
        const treeDrawer = document.getElementById('tree-drawer');
        if (treeBtn && treeDrawer) {
            treeBtn.addEventListener('click', () => {
                treeDrawer.classList.toggle('hidden');
                treeBtn.classList.toggle('active', !treeDrawer.classList.contains('hidden'));
            });
        }
        const treeCloseBtn = document.getElementById('btn-tree-close');
        if (treeCloseBtn && treeDrawer) {
            treeCloseBtn.addEventListener('click', () => {
                treeDrawer.classList.add('hidden');
                if (treeBtn) treeBtn.classList.remove('active');
            });
        }
        const treeResetBtn = document.getElementById('btn-tree-isolate-reset');
        if (treeResetBtn) {
            treeResetBtn.addEventListener('click', () => this.isolateReset());
        }
        const treeSearch = document.getElementById('tree-search-input');
        if (treeSearch) {
            treeSearch.addEventListener('input', (e) => {
                this.treeSearchFilter = e.target.value.trim();
                this.buildModelTreeUI();
            });
        }

        // Theme selector
        const themeSelector = document.getElementById('theme-selector');
        if (themeSelector) {
            themeSelector.addEventListener('change', (e) => {
                this.setTheme(e.target.value);
            });
        }

        // Canvas click for measuring
        this.renderer.domElement.addEventListener('pointerdown', (e) => this.onCanvasClick(e));

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

    toggleMeasure(enable) {
        this.isMeasuring = (enable !== undefined) ? enable : !this.isMeasuring;
        const bar = document.getElementById('measure-bar');
        const btn = document.getElementById('btn-measure');
        if (this.isMeasuring) {
            bar.classList.add('active');
            btn.classList.add('active');
            this.clearMeasure();
        } else {
            bar.classList.remove('active');
            btn.classList.remove('active');
            this.clearMeasure();
        }
    }

    clearMeasure() {
        this.measurePoints = [];
        this.measureMarkers.forEach(m => this.scene.remove(m));
        this.measureMarkers = [];
        if (this.measureLine) {
            this.scene.remove(this.measureLine);
            this.measureLine = null;
        }
        const res = document.getElementById('measure-result');
        if (res) res.innerText = "点击模型拾取两点";
    }

    onCanvasClick(event) {
        if (!this.isMeasuring || !this.currentModel) return;

        const rect = this.renderer.domElement.getBoundingClientRect();
        this.mouse.x = ((event.clientX - rect.left) / rect.width) * 2 - 1;
        this.mouse.y = -((event.clientY - rect.top) / rect.height) * 2 + 1;

        this.raycaster.setFromCamera(this.mouse, this.camera);
        const intersects = this.raycaster.intersectObject(this.currentModel, true);

        if (intersects.length > 0) {
            const pt = intersects[0].point;
            this.addMeasurePoint(pt);
        }
    }

    addMeasurePoint(pt) {
        if (this.measurePoints.length >= 2) {
            this.clearMeasure();
        }

        this.measurePoints.push(pt);

        const markerGeom = new THREE.SphereGeometry(this.modelRadius * 0.015, 16, 16);
        const markerMat = new THREE.MeshBasicMaterial({ color: 0xebcb8b });
        const marker = new THREE.Mesh(markerGeom, markerMat);
        marker.position.copy(pt);
        this.scene.add(marker);
        this.measureMarkers.push(marker);

        if (this.measurePoints.length === 2) {
            const p1 = this.measurePoints[0];
            const p2 = this.measurePoints[1];
            const dist = p1.distanceTo(p2);
            const dx = Math.abs(p2.x - p1.x);
            const dy = Math.abs(p2.y - p1.y);
            const dz = Math.abs(p2.z - p1.z);

            const lineGeom = new THREE.BufferGeometry().setFromPoints([p1, p2]);
            const lineMat = new THREE.LineBasicMaterial({ color: 0xa3be8c, linewidth: 2 });
            this.measureLine = new THREE.Line(lineGeom, lineMat);
            this.scene.add(this.measureLine);

            const res = document.getElementById('measure-result');
            if (res) {
                res.innerText = `${dist.toFixed(2)} mm (ΔX: ${dx.toFixed(2)}, ΔY: ${dy.toFixed(2)}, ΔZ: ${dz.toFixed(2)})`;
            }
        } else {
            const res = document.getElementById('measure-result');
            if (res) res.innerText = "已拾取第 1 点，请点击第 2 点...";
        }
    }

    toggleSection(enable) {
        this.isSectioning = (enable !== undefined) ? enable : !this.isSectioning;
        const bar = document.getElementById('section-bar');
        const btn = document.getElementById('btn-section');
        if (this.isSectioning) {
            bar.classList.add('active');
            btn.classList.add('active');
            this.updateSectionPlane();
        } else {
            bar.classList.remove('active');
            btn.classList.remove('active');
            this.disableSectionPlane();
        }
    }

    setSectionAxis(axis) {
        this.sectionAxis = axis;
        document.querySelectorAll('#section-bar [data-axis]').forEach(b => b.classList.remove('active'));
        const activeBtn = document.querySelector(`#section-bar [data-axis="${axis}"]`);
        if (activeBtn) activeBtn.classList.add('active');
        this.updateSectionPlane();
    }

    updateSectionPlane() {
        if (!this.modelBBox || !this.isSectioning) return;

        let normal = new THREE.Vector3();
        let minVal = 0, maxVal = 0;
        if (this.sectionAxis === 'x') {
            normal.set(this.sectionInvert ? -1 : 1, 0, 0);
            minVal = this.modelBBox.min.x;
            maxVal = this.modelBBox.max.x;
        } else if (this.sectionAxis === 'y') {
            normal.set(0, this.sectionInvert ? -1 : 1, 0);
            minVal = this.modelBBox.min.y;
            maxVal = this.modelBBox.max.y;
        } else if (this.sectionAxis === 'z') {
            normal.set(0, 0, this.sectionInvert ? -1 : 1);
            minVal = this.modelBBox.min.z;
            maxVal = this.modelBBox.max.z;
        }

        const t = this.sectionSliderVal / 100.0;
        const currentCoord = minVal + (maxVal - minVal) * t;

        const constant = -(normal.x * (this.sectionAxis === 'x' ? currentCoord : 0) +
                           normal.y * (this.sectionAxis === 'y' ? currentCoord : 0) +
                           normal.z * (this.sectionAxis === 'z' ? currentCoord : 0));

        this.clipPlane.normal.copy(normal);
        this.clipPlane.constant = constant;

        this.applyClippingPlanes([this.clipPlane]);
    }

    disableSectionPlane() {
        this.applyClippingPlanes([]);
    }

    applyClippingPlanes(planes) {
        if (!this.currentModel) return;
        this.currentModel.traverse((child) => {
            if (child.isMesh && child.material) {
                child.material.clippingPlanes = planes;
                child.material.clipShadows = true;
                child.material.needsUpdate = true;
            }
        });
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

    buildModelTreeUI() {
        const countSpan = document.getElementById('tree-count');
        if (countSpan) countSpan.textContent = this.modelComponents.length;

        const listEl = document.getElementById('tree-list');
        if (!listEl) return;
        listEl.innerHTML = '';

        if (this.modelComponents.length === 0) {
            listEl.innerHTML = '<div style="padding:10px;text-align:center;color:#6c7a89;font-size:11px;">无独立子部件</div>';
            return;
        }

        const filterText = (this.treeSearchFilter || '').toLowerCase();

        this.modelComponents.forEach((comp) => {
            if (filterText && !comp.name.toLowerCase().includes(filterText)) {
                return;
            }

            const item = document.createElement('div');
            item.className = 'tree-item' + (this.selectedComponentId === comp.id ? ' active' : '') + (!comp.visible ? ' hidden-part' : '');
            item.dataset.id = comp.id;

            const eye = document.createElement('span');
            eye.className = 'tree-eye';
            eye.textContent = comp.visible ? '👁️' : '🚫';
            eye.title = comp.visible ? '隐藏该部件' : '显示该部件';
            eye.addEventListener('click', (e) => {
                e.stopPropagation();
                this.toggleComponentVisibility(comp.id);
            });

            const name = document.createElement('span');
            name.className = 'tree-item-name';
            name.textContent = comp.name;
            name.title = comp.name;

            const badge = document.createElement('span');
            badge.className = 'tree-badge';
            badge.textContent = comp.faceCount > 0 ? `${comp.faceCount}面` : `${comp.vertexCount}点`;

            item.appendChild(eye);
            item.appendChild(name);
            item.appendChild(badge);

            item.addEventListener('click', () => {
                this.selectComponent(comp.id);
            });

            listEl.appendChild(item);
        });
    }

    toggleComponentVisibility(id) {
        const comp = this.modelComponents.find(c => c.id === id);
        if (!comp) return;

        comp.visible = !comp.visible;
        comp.object.visible = comp.visible;

        if (this.edgeLines) {
            this.edgeLines.traverse((edgeChild) => {
                if (edgeChild.userData && edgeChild.userData.sourceObject === comp.object) {
                    edgeChild.visible = comp.visible;
                }
            });
        }

        this.buildModelTreeUI();
    }

    selectComponent(id) {
        if (this.selectedComponentId === id) {
            this.selectedComponentId = null;
            this.clearHighlight();
            const statusEl = document.getElementById('tree-status');
            if (statusEl) statusEl.innerHTML = '<span>点击部件选中高亮，点眼睛切换可见性</span>';
            this.buildModelTreeUI();
            return;
        }

        this.selectedComponentId = id;
        const comp = this.modelComponents.find(c => c.id === id);
        if (!comp) return;

        this.highlightComponent(comp.object);

        const statusEl = document.getElementById('tree-status');
        if (statusEl) {
            const compBBox = new THREE.Box3().setFromObject(comp.object);
            const compSize = new THREE.Vector3();
            compBBox.getSize(compSize);
            statusEl.innerHTML = `<span>选中: <b>${comp.name}</b> (${compSize.x.toFixed(1)}×${compSize.y.toFixed(1)}×${compSize.z.toFixed(1)}mm)</span>`;
        }

        this.buildModelTreeUI();
    }

    highlightComponent(object) {
        this.clearHighlight();
        if (!object) return;

        const box = new THREE.BoxHelper(object, 0x00e5ff);
        box.material.depthTest = false;
        box.material.transparent = true;
        box.material.opacity = 0.85;
        this.scene.add(box);
        this.highlightHelper = box;
    }

    clearHighlight() {
        if (this.highlightHelper) {
            this.scene.remove(this.highlightHelper);
            if (this.highlightHelper.geometry) this.highlightHelper.geometry.dispose();
            this.highlightHelper = null;
        }
    }

    isolateReset() {
        this.modelComponents.forEach(comp => {
            comp.visible = true;
            comp.object.visible = true;
        });
        if (this.edgeLines) {
            this.edgeLines.traverse(e => e.visible = true);
        }
        this.selectedComponentId = null;
        this.clearHighlight();
        const statusEl = document.getElementById('tree-status');
        if (statusEl) statusEl.innerHTML = '<span>已恢复所有零部件显示</span>';
        this.buildModelTreeUI();
    }

    setTheme(themeName) {
        this.currentTheme = themeName;
        document.body.className = themeName;
        if (themeName === 'theme-studio') {
            if (this.gridHelper && this.gridHelper.material) this.gridHelper.material.color.setHex(0xaaaaaa);
            if (this.ambientLight) this.ambientLight.intensity = 0.9;
        } else if (themeName === 'theme-blueprint') {
            if (this.gridHelper && this.gridHelper.material) this.gridHelper.material.color.setHex(0x3a608d);
            if (this.ambientLight) this.ambientLight.intensity = 0.65;
        } else {
            if (this.gridHelper && this.gridHelper.material) this.gridHelper.material.color.setHex(0x444b58);
            if (this.ambientLight) this.ambientLight.intensity = 0.65;
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
