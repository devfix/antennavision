import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';
import { Line2 } from 'three/addons/lines/Line2.js';
import { LineMaterial } from 'three/addons/lines/LineMaterial.js';
import { LineGeometry } from 'three/addons/lines/LineGeometry.js';

function invertHexColor(hex) {
    // Remove the '#' if it's present
    const cleanHex = hex.replace('#', '');

    // Convert hex string to an integer
    const colorInt = parseInt(cleanHex, 16);

    // Bitwise XOR with 0xFFFFFF to invert the bits
    const invertedInt = 0xFFFFFF ^ colorInt;

    // Convert back to a 6-character hex string, padded with leading zeros if necessary
    const invertedHex = invertedInt.toString(16).padStart(6, '0');

    return `#${invertedHex.toUpperCase()}`;
}

function getContrastingColor(hexColor) {
    // 1. Remove the '#' if present
    const cleanHex = hexColor.replace('#', '');

    // 2. Parse the r, g, b values from the hex string
    const r = parseInt(cleanHex.substring(0, 2), 16);
    const g = parseInt(cleanHex.substring(2, 4), 16);
    const b = parseInt(cleanHex.substring(4, 6), 16);

    // 3. Calculate the relative luminance
    // These specific multipliers account for human perception (we are highly sensitive to green)
    const luminance = (0.299 * r + 0.587 * g + 0.114 * b) / 255;

    // 4. Return black for light backgrounds, white for dark backgrounds
    // 0.5 is the midpoint brightness threshold
    return luminance > 0.5 ? '#000000' : '#FFFFFF';
}

function load_mesh_objects(scene) {
    configObjects.forEach(obj => {
        let geometry;
        let color = obj.color;
        let type = obj.type.toLowerCase();

        switch(type) {
            case "box":
            geometry = new THREE.BoxGeometry(obj.size[0], obj.size[1], obj.size[2]);
            break;
        case "sphere":
            geometry = new THREE.SphereGeometry(obj.radius, obj.width_segments, obj.height_segments);
            break;
        case "cylinder":
            geometry = new THREE.CylinderGeometry(obj.radius_top, obj.radius_bottom, obj.height, obj.radial_segments);
            break;
        case "cone":
            geometry = new THREE.ConeGeometry(obj.radius, obj.height, obj.radial_segments);
            break;
        case "plane":
            geometry = new THREE.PlaneGeometry(obj.width, obj.height);
            break;
        case "capsule":
            geometry = new THREE.CapsuleGeometry(obj.size[0], obj.size[1], 8, 16);
            break;
        case "torus":
            geometry = new THREE.TorusGeometry(obj.size[0], obj.size[1], 16, 64);
            break;
        default:
            return;
        }

        // Simple material with a wireframe overlay so orientations/edges are clear
        const material = new THREE.MeshLambertMaterial({ color: color });
        const mesh = new THREE.Mesh(geometry, material);

        const wireframeGeom = new THREE.EdgesGeometry(geometry);
        const wireframeMat = new THREE.LineBasicMaterial({ color: getContrastingColor(color), linewidth: 2 });
        const wireframe = new THREE.LineSegments(wireframeGeom, wireframeMat);
        mesh.add(wireframe);

        // Position
        mesh.position.set(obj.pos[0], obj.pos[1], obj.pos[2]);

        // Rotation (Assuming degrees, converting to Radians)
        mesh.quaternion.set(obj.quat[0], obj.quat[1], obj.quat[2], obj.quat[3]);

        scene.add(mesh);
    });
}

function load_line_objects(scene) {
    const lineMaterials = [];
    configObjects.forEach(obj => {
        let geometry;
        let color = obj.color;
        let type = obj.type.toLowerCase();

        if (type === "line") {
            const lineGeo = new LineGeometry();
            if (!obj.points || obj.points.length == 0 || obj.points.length % 3 != 0)
            {
                alert("Invalid points entry for line found!");
                return;
            }

            // Feed the flat [x1, y1, z1, x2, y2, z2...] array to the geometry
            lineGeo.setPositions(obj.points);

            const lineMat = new LineMaterial({
                color: obj.color,
                linewidth: obj.width,
            });

            lineMat.resolution.set(window.innerWidth, window.innerHeight);
            lineMaterials.push(lineMat);

            const lineMesh = new Line2(lineGeo, lineMat);

            scene.add(lineMesh);
        }
    });
}

function load_scene() {

    // 1. Setup Scene, Camera, and Renderer
    const scene = new THREE.Scene();
    scene.add(new THREE.AmbientLight(0xffffff)); // ambient light

    //const dirLight = new THREE.DirectionalLight(0xffffff, 1);
    //dirLight.position.set(10, 20, 15);
    //scene.add(dirLight);

    const camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.001, 1000);
    camera.up.set(0, 0, 1);  // Set the camera's up vector to Z BEFORE initializing OrbitControls
    camera.position.set(0, 0, 0.1);

    const renderer = new THREE.WebGLRenderer({ antialias: true });
    renderer.setSize(window.innerWidth, window.innerHeight);
    document.body.appendChild(renderer.domElement);

    // 2. Add Controls and Helpers
    const controls = new OrbitControls(camera, renderer.domElement);

    const gridHelper = new THREE.GridHelper(100, 100, 0xff0000, 0x444444);
    gridHelper.rotation.x = Math.PI / 2;  // Rotate the grid 90 degrees (Math.PI / 2) around X-axis to lay flat on the X-Y plane
    scene.add(gridHelper); // Visual ground plane reference

    const axesHelper = new THREE.AxesHelper(5);
    scene.add(axesHelper); // Red = X, Green = Y, Blue = Z

    load_mesh_objects(scene);
    load_line_objects(scene);

    // 5. Animation Loop
    function animate() {
        requestAnimationFrame(animate);
        controls.update();
        renderer.render(scene, camera);
    }
    animate();

    // Handle window resizing
    window.addEventListener('resize', () => {
        camera.aspect = window.innerWidth / window.innerHeight;
        camera.updateProjectionMatrix();
        renderer.setSize(window.innerWidth, window.innerHeight);
    });
}

const urlParams = new URLSearchParams(window.location.search);
if (!urlParams.has("path")) {
    alert("missing path parameter");
} else {
    const script = document.createElement('script');
    script.src = urlParams.get("path") + "?t=" + Date.now();  // "Cache Busting" to force the browser to reload the file every time
    script.onerror = function() {
        alert(`Failed to load config file: ${script.src}`);
    };
    document.head.appendChild(script);
    script.onload = load_scene;
}
